//=====[Libraries]=============================================================

#include "esp8266_at.h"

#include "mbed.h"
#include "arm_book_lib.h"
#include "sapi.h"

//=====[Declaration of private defines]========================================

//=====[Declaration of private data types]=====================================

//=====[Declaration and initialization of public global objects]===============

static Serial uartEsp8266( D42, D41 );

//=====[Declaration of external public global variables]=======================

//=====[Declaration and initialization of public global variables]=============

//=====[Declaration and initialization of private global variables]============

static bool flagStartParser = true;
static parser_t parser;
static parserStatus_t parserStatus;

static esp8266State_t esp8266State;

//=====[Declarations (prototypes) of private functions]========================

//=====[Implementations of public functions]===================================

void esp8266UartInit( int baudrate )
{
    uartEsp8266.baud(baudrate);
}

bool esp8266UartByteRead( char* receivedByte )
{
    if( uartEsp8266.readable() ) {
        *receivedByte = uartEsp8266.getc();
        return true;
    }
    return false;
}

void esp8266UartByteWrite( char sendedByte )
{
    uartEsp8266.putc( sendedByte );
}

void esp8266UartStringWrite( char const* str )
{
    while ( *str != NULL ) {
        esp8266UartByteWrite( (uint8_t)*str );
        str++;
    }
}

void esp8266Init()
{
    esp8266UartInit( ESP8266_BAUDRATE );
    esp8266State = ESP8266_IDLE;
}

esp8266Status_t esp8266TestATSend()
{
    if( esp8266State == ESP8266_IDLE ){
        parserInit( &parser, "OK\r\n", strlen("OK\r\n"), 50 );
        parserStart( &parser );
        esp8266UartStringWrite( "AT\r\n" );
        esp8266State = ESP8266_PROCESSING_AT_COMMAND;
        return ESP8266_AT_SENDED;
    } else {        
        return ESP8266_AT_NOT_SENDED;   
    }
}

esp8266Status_t esp8266TestATResponse()
{
    char receivedChar = '\0';
    esp8266UartByteRead( &receivedChar );
    
    parserStatus = parserPatternMatchOrTimeout( &parser, receivedChar );

    switch( parserStatus ) {
        case PARSER_TIMEOUT:
            esp8266State = ESP8266_IDLE;
            return ESP8266_AT_TIMEOUT_WAITING_RESPONSE;
        break;
        case PARSER_PATTERN_MATCH:
            esp8266State = ESP8266_IDLE;
            return ESP8266_AT_RESPONSED;
        break;
        default:
            return ESP8266_AT_PENDING_RESPONSE;
        break;
    }
}

//=====[Implementations of private functions]==================================
