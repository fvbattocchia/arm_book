/*
Modules:
--------

alarm
date_and_time    
event_log    
gas_sensor    
matrix_keypad
pc_serial_communication
smartphone_ble_communication
temperature_sensor

Includes:
---------

#include "mbed.h"
#include "arm_book_lib.h"

#include "alarm.h"

#include "date_and_time.h"

#include "temperature_sensor.h"
#include "gas_sensor.h"

#include "matrix_keypad.h"

#include "pc_serial_communication.h"
#include "smartphone_ble_communication.h"

#include "event_log.h"
*/

//=====[Libraries]=============================================================

#include "mbed.h"
#include "arm_book_lib.h"

//=====[Constants]=============================================================

#define ALARM_CODE_NUMBER_OF_KEYS                4
#define ALARM_BLINKING_TIME_GAS               1000
#define ALARM_BLINKING_TIME_OVER_TEMP          500
#define ALARM_BLINKING_TIME_GAS_AND_OVER_TEMP  100
#define ALARM_OVER_TEMP_LEVEL_CELSIUS         50.0
#define ALARM_GAS_LEVEL                        0.0 // TODO: Ver que valor de umbral poner

#define LM35_SAMPLE_TIME                       100
#define LM35_NUMBER_OF_AVG_SAMPLES              10

#define SYSTEM_TIME_INCREMENT_MS                10

#define MATRIX_KEYPAD_NUMBER_OF_ROWS             4
#define MATRIX_KEYPAD_NUMBER_OF_COLS             4
#define DEBOUNCE_BUTTON_TIME_MS                 40

#define EVENT_LOG_MAX_STORAGE                  100
#define EVENT_LOG_NAME_MAX_LENGTH               15

//=====[Declaration of public data types]======================================

typedef enum {
    MATRIX_KEYPAD_SCANNING,
    MATRIX_KEYPAD_DEBOUNCE,
    MATRIX_KEYPAD_KEY_HOLD_PRESSED
} matrixKeypadState_t;

typedef struct systemEvent {
    time_t seconds;
    char typeOfEvent[EVENT_LOG_NAME_MAX_LENGTH];
} systemEvent_t;

//=====[Declaration and intitalization of public global objects]===============

DigitalIn gasDetector(D2);

DigitalOut alarmLed(LED1);
DigitalOut incorrectCodeLed(LED3);
DigitalOut systemBlockedLed(LED2);

Serial uartUsb(USBTX, USBRX);

Serial uartBle(D1, D0);

AnalogIn lm35(A1);

DigitalOut keypadRowPins[MATRIX_KEYPAD_NUMBER_OF_ROWS] = {D23, D22, D21, D20};
DigitalIn keypadColPins[MATRIX_KEYPAD_NUMBER_OF_COLS]  = {D19, D18, D17, D16};

//=====[Declaration and intitalization of public global variables]=============

static bool gasDetectorState      = OFF;
static bool overTempDetectorState = OFF;
static bool alarmState            = OFF;
static bool incorrectCodeState    = false;
static bool systemBlockedState    = false;
static int numberOfIncorrectCodes = 0;
static char codeSequence[ALARM_CODE_NUMBER_OF_KEYS]   = { '1', '8', '0', '5' };

static float lm35TempC = 0.0;
static float lm35AvgReadingsArray[LM35_NUMBER_OF_AVG_SAMPLES];

static matrixKeypadState_t matrixKeypadState;

static bool alarmLastState = OFF;
static bool gasLastState   = OFF;
static bool tempLastState  = OFF;
static bool ICLastState    = OFF;
static bool SBLastState    = OFF;
static int eventsIndex     = 0;
static systemEvent_t arrayOfStoredEvents[EVENT_LOG_MAX_STORAGE];

//=====[Declarations (prototypes) of public functions]=========================

void smartHomeSystemInit();
void smartHomeSystemUpdate();

void alarmInit();
void alarmUpdate();
bool alarmGasDetectorReadState();
bool alarmOverTempDetectorReadState();
bool alarmReadState();
bool alarmIncorrectCodeReadState();
bool alarmSystemBlockedReadState();
void alarmCodeWrite( char* newCodeSequence );
bool alarmCodeMatch( char* codeToCompare );
static void alarmLedUpdate();
static void incorrectCodeLedUpdate();
static void systemBlockedLedUpdate();
static void alarmActivationUpdate();
static void alarmDeactivationUpdate();
static void alarmCodeCheckFromMatrixKeypad();

void smarphoneBleCommunicationWrite( const char* str );

void pcSerialCommunicationWrite( const char* str );
void pcSerialCommunicationCommandUpdate();
static void availableCommands();
static void commandShowCurrentAlarmState();
static void commandShowCurrentGasDetectorState();
static void commandShowCurrentOverTempDetectorState();
static void commandEnterNewCode();
static void commandShowCurrentTemperatureInCelsius();
static void commandShowCurrentTemperatureInFahrenheit();
static void commandSetDateAndTime();
static void commandShowDateAndTime();
static void commandShowStoredEvents();
static void commandEnterCodeSequence();

void eventLogUpdate();
int eventLogNumberOfStoredEvents();
void eventLogReadStriangAtIndex( int index, char* str );
void eventLogWrite( bool currentState, const char* elementName );
static void eventLogElementStateUpdate( bool lastState,
                                        bool currentState,
                                        const char* elementName );

char* dateAndTimeReadString();
void dateAndTimeWriteIndividualValues( int year, int month, int day, 
                                       int hour, int minute, int second );

void gasSensorInit( void );
void gasSensorUpdate( void );
float gasSensorRead();

void temperatureSensorInit( void );
void temperatureSensorUpdate( void );
float temperatureSensorReadCelsius( void );
float temperatureSensorReadFahrenheit( void );
float celsiusToFahrenheit( float tempInCelsiusDegrees );
static float analogReadingScaledWithTheLM35Formula( float analogReading );
static void shiftLm35AvgReadingsArray();

void matrixKeypadInit();
static char matrixKeypadScan();
char matrixKeypadUpdate();

//=====[Main function, the program entry point after power on or reset]========

int main()
{
    smartHomeSystemInit();
    while (true) {
        smartHomeSystemUpdate();
    }
}

//=====[Implementations of public functions]===================================

void smartHomeSystemInit()
{
    alarmInit();
    matrixKeypadInit();
}

void smartHomeSystemUpdate()
{
    alarmUpdate();
    pcSerialCommunicationCommandUpdate();
    eventLogUpdate();
    delay(SYSTEM_TIME_INCREMENT_MS);
}

void alarmInit()
{
    alarmLed = OFF;
    incorrectCodeLed = OFF;
    systemBlockedLed = OFF;
    gasSensorInit();
    temperatureSensorInit();
}

void alarmUpdate()
{
    alarmActivationUpdate();
    alarmDeactivationUpdate();
}

bool alarmGasDetectorReadState()
{
    return gasDetectorState;
}

bool alarmOverTempDetectorReadState()
{
    return overTempDetectorState;
}

bool alarmReadState()
{
    return alarmState;
}

bool alarmIncorrectCodeReadState()
{
    return incorrectCodeState;
}

bool alarmSystemBlockedReadState()
{
    return systemBlockedState;
}

void alarmCodeWrite( char* newCodeSequence )
{
    for ( int i = 0; i < ALARM_CODE_NUMBER_OF_KEYS; i++) {
        codeSequence[i] = newCodeSequence[i];
    }
}

bool alarmCodeMatch( char* codeToCompare )
{
    int i;

    for (i = 0; i < ALARM_CODE_NUMBER_OF_KEYS; i++) {
        if ( codeSequence[i] != codeToCompare[i] ) {
            incorrectCodeState = true;
            numberOfIncorrectCodes = numberOfIncorrectCodes + 1;
            return false;
        }
    }

    incorrectCodeState = false;
    alarmState = OFF;
    numberOfIncorrectCodes = 0;
    return true;
}

static void alarmLedUpdate()
{
    static int accumulatedTimeAlarm = 0;
    accumulatedTimeAlarm = accumulatedTimeAlarm + SYSTEM_TIME_INCREMENT_MS;
    
    if( alarmState ) {
        if( gasDetectorState && overTempDetectorState ) {
            if( accumulatedTimeAlarm >= ALARM_BLINKING_TIME_GAS_AND_OVER_TEMP ) {
                accumulatedTimeAlarm = 0;
                alarmLed = !alarmLed;
            }
        } else if( gasDetectorState ) {
            if( accumulatedTimeAlarm >= ALARM_BLINKING_TIME_GAS ) {
                accumulatedTimeAlarm = 0;
                alarmLed = !alarmLed;
            }
        } else if ( overTempDetectorState ) {
            if( accumulatedTimeAlarm >= ALARM_BLINKING_TIME_OVER_TEMP  ) {
                accumulatedTimeAlarm = 0;
                alarmLed = !alarmLed;
            }
        }
    } else {
        alarmLed = OFF;
    }
}

static void incorrectCodeLedUpdate()
{
    incorrectCodeLed = incorrectCodeState;
}

static void systemBlockedLedUpdate()
{
    systemBlockedLed = systemBlockedState;
}

static void alarmActivationUpdate()
{
    temperatureSensorUpdate();
    if ( temperatureSensorReadCelsius() > ALARM_OVER_TEMP_LEVEL_CELSIUS ) {
        overTempDetectorState = ON;
        alarmState = ON;
    }

    gasSensorUpdate();
    if ( gasSensorRead() > ALARM_GAS_LEVEL ) {
        gasDetectorState = ON;
        alarmState = ON;
    }

    if( alarmState == OFF ) {
        gasDetectorState = OFF;
        overTempDetectorState = OFF;
    }

    alarmLedUpdate();
}

static void alarmCodeCheckFromMatrixKeypad()
{
    static int matrixKeypadCodeIndex = 0;
    static int numberOfEnterButtonReleased = 0;
    static char buttonsPressed[ALARM_CODE_NUMBER_OF_KEYS] = {'0','0','0','0'};

    char keyReleased = matrixKeypadUpdate();

    if( keyReleased != '\0' && keyReleased != '#' ) {
        buttonsPressed[matrixKeypadCodeIndex] = keyReleased;
        if( matrixKeypadCodeIndex >= ALARM_CODE_NUMBER_OF_KEYS ) {
            matrixKeypadCodeIndex = 0;
        } else {
            matrixKeypadCodeIndex++;
        }
    }

    if( keyReleased == '#' ) {
        if( incorrectCodeState ) {
            numberOfEnterButtonReleased++;
            if( numberOfEnterButtonReleased >= 2 ) {
                numberOfEnterButtonReleased = 0;
                matrixKeypadCodeIndex = 0;
                incorrectCodeState = OFF;
            }
        } else {
            if ( alarmState ) {
                alarmCodeMatch(buttonsPressed);
            }
        }
    }
}

static void alarmDeactivationUpdate()
{
    if ( numberOfIncorrectCodes < 5 ) {
        alarmCodeCheckFromMatrixKeypad();
    } else {
        systemBlockedState = ON;
    }
    incorrectCodeLedUpdate();
    systemBlockedLedUpdate();
}

char* dateAndTimeReadString()
{
    time_t epochSeconds;
    epochSeconds = time(NULL);
    return ctime(&epochSeconds);    
}

void dateAndTimeWriteIndividualValues( int year, int month, int day, 
                                       int hour, int minute, int second )
{
    struct tm rtcTime;

    rtcTime.tm_year = year - 1900;
    rtcTime.tm_mon  = month - 1;
    rtcTime.tm_mday = day;
    rtcTime.tm_hour = hour;
    rtcTime.tm_min  = minute;
    rtcTime.tm_sec  = second;

    rtcTime.tm_isdst = -1;

    set_time( mktime( &rtcTime ) );
}

void gasSensorInit( void )
{
    gasDetector.mode(PullDown);
}

void gasSensorUpdate( void )
{
    return;
}

float gasSensorRead()
{
    return (float)gasDetector;
}

void temperatureSensorInit( void )
{
    return;
}

void temperatureSensorUpdate( void )
{
    static int accumulatedTimeLm35 = 0;
    static int lm35SampleIndex     = 0;
    static float lm35ReadingsMovingAverage = 0.0;

    accumulatedTimeLm35 = accumulatedTimeLm35 + SYSTEM_TIME_INCREMENT_MS;

    if ( accumulatedTimeLm35 >= LM35_SAMPLE_TIME ) {
        if ( lm35SampleIndex < LM35_NUMBER_OF_AVG_SAMPLES ) {
            lm35AvgReadingsArray[lm35SampleIndex] = lm35.read() / LM35_NUMBER_OF_AVG_SAMPLES;
            lm35ReadingsMovingAverage = lm35ReadingsMovingAverage +
                                        lm35AvgReadingsArray[lm35SampleIndex];
            lm35SampleIndex++;
        } else {
            lm35ReadingsMovingAverage = lm35ReadingsMovingAverage -
                                        lm35AvgReadingsArray[0];

            shiftLm35AvgReadingsArray();

            lm35AvgReadingsArray[LM35_NUMBER_OF_AVG_SAMPLES-1] =
                lm35.read() / LM35_NUMBER_OF_AVG_SAMPLES;

            lm35ReadingsMovingAverage =
                lm35ReadingsMovingAverage +
                lm35AvgReadingsArray[LM35_NUMBER_OF_AVG_SAMPLES-1];

            lm35TempC = analogReadingScaledWithTheLM35Formula(
                            lm35ReadingsMovingAverage );
        }
        accumulatedTimeLm35 = 0;
    }
}

float temperatureSensorReadCelsius( void )
{
    return lm35TempC;
}

float temperatureSensorReadFahrenheit( void )
{
    return celsiusToFahrenheit( lm35TempC );
}

void smarphoneBleCommunicationWrite( const char* str )
{
    uartBle.printf( "%s", str );
}

void pcSerialCommunicationWrite( const char* str )
{
    uartUsb.printf( "%s", str );
}

void pcSerialCommunicationCommandUpdate()
{
    if( uartUsb.readable() ) {
        char receivedChar = uartUsb.getc();

        switch (receivedChar) {
            case '1': commandShowCurrentAlarmState(); break;
            case '2': commandShowCurrentGasDetectorState(); break;
            case '3': commandShowCurrentOverTempDetectorState(); break;
            case '4': commandEnterCodeSequence(); break;
            case '5': commandEnterNewCode(); break;
            case 'c': case 'C': commandShowCurrentTemperatureInCelsius(); break;
            case 'f': case 'F': commandShowCurrentTemperatureInFahrenheit(); break;
            case 's': case 'S': commandSetDateAndTime(); break;
            case 't': case 'T': commandShowDateAndTime(); break;
            case 'e': case 'E': commandShowStoredEvents(); break;
            default: availableCommands(); break;
        }
    }
}

static void availableCommands()
{
    uartUsb.printf( "Available commands:\r\n" );
    uartUsb.printf( "Press '1' to get the alarm state\r\n" );
    uartUsb.printf( "Press '2' for gas detector state\r\n" );
    uartUsb.printf( "Press '3' for over temperature detector state\r\n" );
    uartUsb.printf( "Press '4' to enter the code sequence\r\n" );
    uartUsb.printf( "Press '5' to enter a new code\r\n" );
    uartUsb.printf( "Press 'f' or 'F' to get lm35 reading in Fahrenheit\r\n" );
    uartUsb.printf( "Press 'c' or 'C' to get lm35 reading in Celsius\r\n" );
    uartUsb.printf( "Press 's' or 'S' to set the date and time\r\n" );
    uartUsb.printf( "Press 't' or 'T' to get the date and time\r\n" );
    uartUsb.printf( "Press 'e' or 'E' to get the stored events\r\n" );
    uartUsb.printf( "\r\n" );
}

static void commandShowCurrentAlarmState()
{
    if ( alarmReadState() ) {
        uartUsb.printf( "The alarmLed is activated\r\n");
    } else {
        uartUsb.printf( "The alarmLed is not activated\r\n");
    }
}

static void commandShowCurrentGasDetectorState()
{
    if ( alarmGasDetectorReadState() ) {
        uartUsb.printf( "Gas is being detected\r\n");
    } else {
        uartUsb.printf( "Gas is not being detected\r\n");
    }    
}

static void commandShowCurrentOverTempDetectorState()
{
    if ( alarmOverTempDetectorReadState() ) {
        uartUsb.printf( "Temperature is above the maximum level\r\n");
    } else {
        uartUsb.printf( "Temperature is below the maximum level\r\n");
    }
}

static void commandEnterCodeSequence()
{
    char receivedCodeSequence[ALARM_CODE_NUMBER_OF_KEYS];
    
    uartUsb.printf( "Please enter the new four digits numeric code " );
    uartUsb.printf( "to deactivate the alarm.\r\n" );

    for ( int i = 0; i < ALARM_CODE_NUMBER_OF_KEYS; i++) {
        receivedCodeSequence[i] = uartUsb.getc();
        uartUsb.printf( "*" );
    }

    if ( alarmCodeMatch(receivedCodeSequence) ) {
        uartUsb.printf( "\r\nThe code is correct\r\n\r\n" );
    } else {
        uartUsb.printf( "\r\nThe code is incorrect\r\n\r\n" );
    }    
}

static void commandEnterNewCode()
{
    char newCodeSequence[ALARM_CODE_NUMBER_OF_KEYS];

    uartUsb.printf( "Please enter the new four digits numeric code\r\n" );

    for ( int i = 0; i < ALARM_CODE_NUMBER_OF_KEYS; i++) {
        newCodeSequence[i] = uartUsb.getc();
        uartUsb.printf( "*" );
    }

    alarmCodeWrite( newCodeSequence );

    uartUsb.printf( "\r\nNew code configurated\r\n\r\n" ); 
}

static void commandShowCurrentTemperatureInCelsius()
{
    uartUsb.printf( "Temperature: %.2f �C\r\n",
                    temperatureSensorReadCelsius() );    
}

static void commandShowCurrentTemperatureInFahrenheit()
{
    uartUsb.printf( "Temperature: %.2f �F\r\n", 
                    temperatureSensorReadFahrenheit() );    
}

static void commandSetDateAndTime()
{
    int year   = 0;
    int month  = 0;
    int day    = 0;
    int hour   = 0;
    int minute = 0;
    int second = 0;
    
    uartUsb.printf("Enter the current year (YYYY): ");
    uartUsb.scanf("%d", &year);
    uartUsb.printf("%d\r\n", year);

    uartUsb.printf("Enter the current month (1-12): ");
    uartUsb.scanf("%d", &month);
    uartUsb.printf("%d\r\n", month);

    uartUsb.printf("Enter the current day (1-31): ");
    uartUsb.scanf("%d", &day);
    uartUsb.printf("%d\r\n", day);

    uartUsb.printf("Enter the current hour (0-24): ");
    uartUsb.scanf("%d", &hour);
    uartUsb.printf("%d\r\n",hour);

    uartUsb.printf("Enter the current minute (0-59): ");
    uartUsb.scanf("%d", &minute);
    uartUsb.printf("%d\r\n", minute);

    uartUsb.printf("Enter the current second (0-59): ");
    uartUsb.scanf("%d", &second);
    uartUsb.printf("%d\r\n", second);

    while ( uartUsb.readable() ) {
        uartUsb.getc();
    }

    dateAndTimeWriteIndividualValues( year, month, day, 
                                      hour, minute, second );
}

static void commandShowDateAndTime()
{
    uartUsb.printf("Date and Time = %s", dateAndTimeReadString());
}

static void commandShowStoredEvents()
{
    char str[100];
    for (int i = 0; i < eventLogNumberOfStoredEvents(); i++) {
        eventLogReadStriangAtIndex( i, str );
        uartUsb.printf( "%s\r\n", str );                       
    }
}

void eventLogUpdate()
{
    bool currentState = alarmReadState();
    eventLogElementStateUpdate( alarmLastState, currentState, "ALARM" );
    alarmLastState = currentState;

    currentState = alarmGasDetectorReadState();
    eventLogElementStateUpdate( gasLastState, currentState, "GAS_DET" );
    gasLastState = currentState;

    currentState = alarmOverTempDetectorReadState();
    eventLogElementStateUpdate( tempLastState, currentState, "OVER_TEMP" );
    tempLastState = currentState;

    currentState = alarmIncorrectCodeReadState();
    eventLogElementStateUpdate( ICLastState, currentState, "LED_IC" );
    ICLastState = currentState;

    currentState = alarmSystemBlockedReadState();
    eventLogElementStateUpdate( SBLastState ,currentState, "LED_SB" );
    SBLastState = currentState;
}

int eventLogNumberOfStoredEvents()
{
    return eventsIndex;
}

void eventLogReadStriangAtIndex( int index, char* str )
{
    str[0] = 0;

    strncat( str, "Event = ", strlen("Event = ") );

    strncat( str, arrayOfStoredEvents[index].typeOfEvent,
             strlen(arrayOfStoredEvents[index].typeOfEvent) );

    strncat( str, "\r\nDate and Time = ", strlen("\r\nDate and Time = ") );

    strncat( str, ctime(&arrayOfStoredEvents[index].seconds),
             strlen(ctime(&arrayOfStoredEvents[index].seconds)) );

    strncat( str, "\r\n", strlen("\r\n") );
}

void eventLogWrite( bool currentState, const char* elementName )
{
    char eventAndStateStr[EVENT_LOG_NAME_MAX_LENGTH];
    eventAndStateStr[0] = 0;
    strncat( eventAndStateStr, elementName, strlen(elementName) );
    if ( currentState ) {
        strncat( eventAndStateStr, "_ON", strlen("_ON") );
    } else {
        strncat( eventAndStateStr, "_OFF", strlen("_OFF") );
    }

    arrayOfStoredEvents[eventsIndex].seconds = time(NULL);
    strcpy( arrayOfStoredEvents[eventsIndex].typeOfEvent, eventAndStateStr );
    if ( eventsIndex < EVENT_LOG_MAX_STORAGE ) {
        eventsIndex++;
    } else {
        eventsIndex = 0;
    }

    pcSerialCommunicationWrite(eventAndStateStr);
    pcSerialCommunicationWrite("\r\n");
 
    smarphoneBleCommunicationWrite(eventAndStateStr);
    smarphoneBleCommunicationWrite("\r\n");
}

static void eventLogElementStateUpdate( bool lastState,
                                        bool currentState,
                                        const char* elementName )
{
    if ( lastState != currentState ) {        
        eventLogWrite( currentState, elementName );       
    }
}

static float analogReadingScaledWithTheLM35Formula( float analogReading )
{
    return ( analogReading * 3.3 / 0.01 );
}

float celsiusToFahrenheit( float tempInCelsiusDegrees )
{
    return ( tempInCelsiusDegrees * 9.0 / 5.0 + 32.0 );
}

static void shiftLm35AvgReadingsArray()
{
    int i = 0;
    for( i=1; i<LM35_NUMBER_OF_AVG_SAMPLES; i++ ) {
        lm35AvgReadingsArray[i-1] = lm35AvgReadingsArray[i];
    }
    lm35AvgReadingsArray[LM35_NUMBER_OF_AVG_SAMPLES-1] = 0.0;
}

void matrixKeypadInit()
{
    matrixKeypadState = MATRIX_KEYPAD_SCANNING;
    int pinIndex = 0;
    for( pinIndex=0; pinIndex<MATRIX_KEYPAD_NUMBER_OF_COLS; pinIndex++ ) {
        (keypadColPins[pinIndex]).mode(PullUp);
    }
}

static char matrixKeypadScan()
{
    int r = 0;
    int c = 0;
    int i = 0; 

    char matrixKeypadIndexToCharArray[] = {
        '1', '2', '3', 'A',
        '4', '5', '6', 'B',
        '7', '8', '9', 'C',
        '*', '0', '#', 'D',
    };

    for( r=0; r<MATRIX_KEYPAD_NUMBER_OF_ROWS; r++ ) {

        for( i=0; i<MATRIX_KEYPAD_NUMBER_OF_ROWS; i++ ) {
            keypadRowPins[i] = ON;
        }

        keypadRowPins[r] = OFF;

        for( c=0; c<MATRIX_KEYPAD_NUMBER_OF_COLS; c++ ) {
            if( keypadColPins[c] == OFF ) {
                return matrixKeypadIndexToCharArray[r*MATRIX_KEYPAD_NUMBER_OF_ROWS + c];
            }
        }
    }
    return '\0';
}

char matrixKeypadUpdate()
{
    static int accumulatedDebounceMatrixKeypadTime = 0;
    static char matrixKeypadLastKeyPressed = '\0';

    char keyDetected = '\0';
    char keyReleased = '\0';

    switch( matrixKeypadState ) {

    case MATRIX_KEYPAD_SCANNING:
        keyDetected = matrixKeypadScan();
        if( keyDetected != '\0' ) {
            matrixKeypadLastKeyPressed = keyDetected;
            accumulatedDebounceMatrixKeypadTime = 0;
            matrixKeypadState = MATRIX_KEYPAD_DEBOUNCE;
        }
        break;

    case MATRIX_KEYPAD_DEBOUNCE:
        if( accumulatedDebounceMatrixKeypadTime >=
            DEBOUNCE_BUTTON_TIME_MS ) {
            keyDetected = matrixKeypadScan();
            if( keyDetected == matrixKeypadLastKeyPressed ) {
                matrixKeypadState = MATRIX_KEYPAD_KEY_HOLD_PRESSED;
            } else {
                matrixKeypadState = MATRIX_KEYPAD_SCANNING;
            }
        }
        accumulatedDebounceMatrixKeypadTime =
            accumulatedDebounceMatrixKeypadTime + SYSTEM_TIME_INCREMENT_MS;
        break;

    case MATRIX_KEYPAD_KEY_HOLD_PRESSED:
        keyDetected = matrixKeypadScan();
        if( keyDetected != matrixKeypadLastKeyPressed ) {
            if( keyDetected == '\0' ) {
                keyReleased = matrixKeypadLastKeyPressed;
            }
            matrixKeypadState = MATRIX_KEYPAD_SCANNING;
        }
        break;

    default:
        matrixKeypadInit();
        break;
    }
    return keyReleased;
}