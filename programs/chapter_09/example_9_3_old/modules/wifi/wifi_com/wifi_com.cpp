//=====[Libraries]=============================================================

#include "wifi_com.h"

#include "sapi.h"
#include "pc_serial_com.h"
#include "wifi_module.h"
#include "wifi_credentials.h"
#include "esp8266_at.h"


//=====[Declaration of private defines]========================================

//=====[Declaration of private data types]=====================================

typedef enum{
    WIFI_STATE_MODULE_DETECT,
    WIFI_STATE_MODULE_NOT_DETECTED,
    WIFI_STATE_MODULE_INIT,
    WIFI_STATE_MODULE_CHECK_AP_CONNECTION,
    WIFI_STATE_MODULE_NOT_CONNECTED,
    WIFI_STATE_COMMUNICATION_INIT,
    WIFI_STATE_COMMUNICATION_UPDATE,
} wifiFsmComState_t;

//=====[Declaration and initialization of public global objects]===============

//=====[Declaration of external public global variables]=======================

//=====[Declaration and initialization of public global variables]=============

//=====[Declaration and initialization of private global variables]============

static wifiFsmComState_t wifiComFsmState;

//=====[Declarations (prototypes) of private functions]========================

static void runStateWifiModuleDetect();
static void runStateWifiModuleNotDetected();

static void runStateWifiModuleInit();

static void runStateWifiModuleCheckAPConnection();
static void runStateWifiModuleNotConnected();

static void runStateWifiCommunicationInit();
static void runStateWifiCommunicationUpdate();

//=====[Implementations of public functions]===================================

// Wi-Fi FSM ------------------------------------------------------------------

void wifiComInit()
{
    wifiComFsmState = WIFI_STATE_MODULE_DETECT;    
    esp8266Init();
}

void wifiComUpdate()
{
    switch ( wifiComFsmState ) {
        case WIFI_STATE_MODULE_DETECT: 
            runStateWifiModuleDetect();
        break;
        case WIFI_STATE_MODULE_NOT_DETECTED:
            runStateWifiModuleNotDetected();
        break;
        case WIFI_STATE_MODULE_INIT:
            runStateWifiModuleInit();
        break;
        case WIFI_STATE_MODULE_CHECK_AP_CONNECTION:
            runStateWifiModuleCheckAPConnection();
        break;
        case WIFI_STATE_MODULE_NOT_CONNECTED:
            runStateWifiModuleNotConnected();
        break;
        case WIFI_STATE_COMMUNICATION_INIT:
            runStateWifiCommunicationInit();
        break;
        case WIFI_STATE_COMMUNICATION_UPDATE:
            runStateWifiCommunicationUpdate();
        break;
        default:
            wifiComFsmState = WIFI_STATE_MODULE_DETECT;
        break;
    } 
}

//=====[Implementations of private functions]==================================

// En este estado si detecta el modulo pasa al estado 
// WIFI_STATE_MODULE_INIT.
// Si no pasa al estado: WIFI_STATE_MODULE_NOT_DETECTED
static void runStateWifiModuleDetect()
{
    static bool stateEntryFlag = false;
    // ENTRY ----------------------------------------
    if( stateEntryFlag == false ){
        if( wifiModuleStartDetection() != WIFI_MODULE_DETECTION_STARTED ){
            return;
        }
        stateEntryFlag = true;
    }

    // CHECK TRANSITION CONDITIONS ------------------
    switch( wifiModuleDetectionResponse() ) {
        case WIFI_MODULE_DETECTED:
            wifiComFsmState = WIFI_STATE_MODULE_INIT;
            pcSerialComStringWrite( "Wi-Fi module detected.\r\n" );
        break;
        case WIFI_MODULE_NOT_DETECTED:
            wifiComFsmState = WIFI_STATE_MODULE_NOT_DETECTED;
        break;
        default: // Module busy, not do anything
        break;
    }

    // EXIT ------------------------------------------
    if( wifiComFsmState != WIFI_STATE_MODULE_DETECT ){
        stateEntryFlag = false;
    }
}

// En este estado reintenta detectar el módulo Wi-Fi cada 10 segundos (lo resetea).
// Cuando lo detecta pasa al estado WIFI_STATE_MODULE_INIT
static void runStateWifiModuleNotDetected()
{
    static bool stateEntryFlag = false;
    static bool resetSended = false;
    static delay_t reintentsDelay;
    // ENTRY ----------------------------------------
    if( stateEntryFlag == false ) {
        pcSerialComStringWrite( "\r\nERROR: Wi-Fi module not detected!\r\n" );
        pcSerialComStringWrite( "It will re-intent automaticaly in 10 seconds...\r\n" );
        delayInit( &reintentsDelay, 10000 );
        stateEntryFlag = true;
    }

    // CHECK TRANSITION CONDITIONS ------------------
    if( delayRead(&reintentsDelay) && !resetSended ) {
        pcSerialComStringWrite( "Reseting Wi-Fi module..\r\n" );
        if( wifiModuleStartReset() == WIFI_MODULE_RESET_STARTED ) {
            resetSended = true;
        }
    }
    if ( resetSended ) {            
        switch( wifiModuleResetResponse() ) {
            case WIFI_MODULE_RESET_COMPLETE:
                wifiComFsmState = WIFI_STATE_MODULE_INIT;
                pcSerialComStringWrite( "Wi-Fi module detected.\r\n" );
            break;
            case WIFI_MODULE_NOT_DETECTED:
                wifiComFsmState = WIFI_STATE_MODULE_NOT_DETECTED;
                stateEntryFlag = false; // Asi relanzo el entry de este estado   
            break;
            default: // Module busy, not do anything
            break;
        }
        resetSended = false;   
    }

    // EXIT ------------------------------------------
    if( wifiComFsmState != WIFI_STATE_MODULE_NOT_DETECTED ){
        stateEntryFlag = false;
    }
}

// En este estado inicializa el modulo Wi-Fi para poder conectarse a un AP
// Cuando termina la inicializacion pasa al estado
// WIFI_STATE_MODULE_CHECK_AP_CONNECTION
static void runStateWifiModuleInit()
{
    static bool stateEntryFlag = false;
    // ENTRY ----------------------------------------
    if( stateEntryFlag == false ){
        if( wifiModuleStartInit() != WIFI_MODULE_INIT_STARTED ){
            return;
        }
        stateEntryFlag = true;
    }

    // CHECK TRANSITION CONDITIONS ------------------
    switch( wifiModuleInitResponse() ) {
        case WIFI_MODULE_INIT_COMPLETE:
            pcSerialComStringWrite( "Wi-Fi module initialized.\r\n" );
            wifiComFsmState = WIFI_STATE_MODULE_CHECK_AP_CONNECTION;
        break;
        case WIFI_MODULE_NOT_DETECTED:
            wifiComFsmState = WIFI_STATE_MODULE_NOT_DETECTED;
        break;
        default: // Module busy, not do anything
        break;
    }

    // EXIT ------------------------------------------
    if( wifiComFsmState != WIFI_STATE_MODULE_INIT ){
        stateEntryFlag = false;
    }
}

// Chequea si esta conectado y tiene IP.
// Si está conectado y tiene IP pasa al estado WIFI_STATE_COMMUNICATIONS_INIT
// Si no está conectado pasa al estado: WIFI_STATE_MODULE_NOT_CONNECTED
static void runStateWifiModuleCheckAPConnection()
{
    static bool stateEntryFlag = false;
    char ip[20] = ""; 
    // ENTRY ----------------------------------------
    if( stateEntryFlag == false ){
        if( wifiModuleStartIsConnectedWithAP() != WIFI_MODULE_INIT_STARTED ){
            return;
        }
        stateEntryFlag = true;
    }

    // CHECK TRANSITION CONDITIONS ------------------
    switch( wifiModuleIsConnectedWithAPResponse( ip ) ) {
        case WIFI_MODULE_IS_CONNECTED:
            pcSerialComStringWrite( "Wi-Fi module is connected. IP = " );
            pcSerialComStringWrite( ip );
            pcSerialComStringWrite( "\r\n" );
            wifiComFsmState = WIFI_STATE_COMMUNICATION_INIT;
        break;
        case WIFI_MODULE_IS_NOT_CONNECTED:
            pcSerialComStringWrite( "Wi-Fi module is not connected.\r\n" );
            wifiComFsmState = WIFI_STATE_MODULE_NOT_CONNECTED;
        break;
        case WIFI_MODULE_NOT_DETECTED:
            wifiComFsmState = WIFI_STATE_MODULE_NOT_DETECTED;
        break;
        default: // Module busy, not do anything
        break;
    }

    // EXIT ------------------------------------------
    if( wifiComFsmState != WIFI_STATE_MODULE_CHECK_AP_CONNECTION ){
        stateEntryFlag = false;
    }
}

// Si no está conectado reintenta conectarse cada 10 segundos.
// Si no detecta el módulo vuelve a: 
//     WIFI_STATE_MODULE_NOT_DETECTED.
static void runStateWifiModuleNotConnected()
{
    static bool stateEntryFlag = false;
    char ip[20] = ""; 
    static bool resetSended = false;
    static delay_t reintentsDelay;
    // ENTRY ----------------------------------------
    if( stateEntryFlag == false ){
        if( wifiModuleStartConnectWithAP( WIFI_SSID , WIFI_PASSWORD ) !=
            WIFI_MODULE_CONNECT_AP_STARTED ){
            return;
        }        
        pcSerialComStringWrite( "Wi-Fi try to connect with: " );
        pcSerialComStringWrite( WIFI_SSID );
        pcSerialComStringWrite( "\r\n" );
        stateEntryFlag = true;
    }

    // CHECK TRANSITION CONDITIONS ------------------
    if( delayRead(&reintentsDelay) && !resetSended ) {
        stateEntryFlag = false; // Asi relanzo el entry de este estado
    }

    switch( wifiModuleConnectWithAPResponse( ip ) ) {
        case WIFI_MODULE_IS_CONNECTED:
            pcSerialComStringWrite( "Wi-Fi module is connected. IP = " );
            pcSerialComStringWrite( ip );
            pcSerialComStringWrite( "\r\n" );
            wifiComFsmState = WIFI_STATE_COMMUNICATION_INIT;
        break;
        case WIFI_MODULE_NOT_DETECTED:
            wifiComFsmState = WIFI_STATE_MODULE_NOT_DETECTED;          
        break;
        case WIFI_MODULE_BUSY: // Module busy, not do anything
        break;
        default: // Error trying to connect
            pcSerialComStringWrite( "\r\nERROR: Wi-Fi not connected!\r\n" );
            pcSerialComStringWrite( "It will re-intent automaticaly in 10 seconds...\r\n" );
            delayInit( &reintentsDelay, 10000 );
        break;
    }

    // EXIT ------------------------------------------
    if( wifiComFsmState != WIFI_STATE_MODULE_NOT_CONNECTED ){
        stateEntryFlag = false;
    }
}


// En este estado:
//     embeddedServerInit(); // Chapter 9 y 10
//     clientInit();         // Chapter 10
//     Y  cuando termina pasa al estado:
//        WIFI_STATE_COMMUNICATION_UPDATE
// En este estado levanta el Server y se pasa al siguiente estado.
// Si no detecta el módulo vuelve a: 
//     WIFI_STATE_MODULE_NOT_DETECTED.
// Si se pierde la conexión entre el módulo y el AP vuelve a: 
//     WIFI_STATE_MODULE_WAIT_FOR_AP_CONNECTION
static void runStateWifiCommunicationInit()
{
    static bool stateEntryFlag = false;
    // ENTRY ----------------------------------------
    if( stateEntryFlag == false ){
        stateEntryFlag = true;
        pcSerialComStringWrite( "Wi-Fi communication initialized\r\n" );
    }

    // UPDATE OUTPUTS -------------------------------


    // CHECK TRANSITION CONDITIONS ------------------
    wifiComFsmState != WIFI_STATE_COMMUNICATION_INIT;

    // EXIT ------------------------------------------
    if( wifiComFsmState != WIFI_STATE_COMMUNICATION_UPDATE ){
        stateEntryFlag = false;
    }
}

// embeddedServerUpdate(); // Chapter 9 y 10: FSM de Embedded Server
// clientUpdate();         // Chapter 10: FSM de Client
// Si no detecta el módulo vuelve a: 
//     WIFI_STATE_MODULE_NOT_DETECTED.
// Si se pierde la conexión entre el módulo y el AP vuelve a: 
//     WIFI_STATE_MODULE_WAIT_FOR_AP_CONNECTION
static void runStateWifiCommunicationUpdate()
{
    static bool stateEntryFlag = false;
    // ENTRY ----------------------------------------
    if( stateEntryFlag == false ){
        stateEntryFlag = true;
        pcSerialComStringWrite( "Wi-Fi communication Update\r\n" );
    }

    // UPDATE OUTPUTS -------------------------------


    // CHECK TRANSITION CONDITIONS ------------------


    // EXIT ------------------------------------------
    if( wifiComFsmState != WIFI_STATE_COMMUNICATION_UPDATE ){
        stateEntryFlag = false;
    }
}