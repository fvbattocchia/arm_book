//=====[Libraries]=============================================================

#include "wifi_module.h"

#include "wifi_credentials.h"
#include "sapi.h"
#include "pc_serial_com.h"

//=====[Declaration of private defines]========================================

//=====[Declaration of private data types]=====================================

//=====[Declaration and initialization of public global objects]===============

//=====[Declaration of external public global variables]=======================

//=====[Declaration and initialization of public global variables]=============

//=====[Declaration and initialization of private global variables]============

//=====[Declarations (prototypes) of private functions]========================

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
        case WIFI_STATE_MODULE_WAIT_FOR_AP_CONNECTION:
            runStateWifiModuleWaitForAPConnection();
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
// WIFI_STATE_MODULE_WAIT_FOR_AP_CONNECTION.
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
            wifiComFsmState = WIFI_STATE_MODULE_WAIT_FOR_AP_CONNECTION;
        break;
        case WIFI_MODULE_NOT_DETECTED:
            wifiComFsmState = WIFI_STATE_MODULE_NOT_DETECTED;
        break;
        default:
        break;
    }
    // EXIT ------------------------------------------
    if( wifiComFsmState != WIFI_STATE_MODULE_DETECT ){
        stateEntryFlag = false;
    }
}

// En este estado reintenta detectar el módulo Wi-Fi cada 10 segundos (lo resetea).
// Cuando lo detecta pasa al estado WIFI_STATE_MODULE_WAIT_FOR_AP_CONNECTION
static void runStateWifiModuleNotDetected()
{
    static bool stateEntryFlag = false;
    static bool resetSended = false;
    static delay_t reintentsDelay;
    // ENTRY ----------------------------------------
    if( stateEntryFlag == false ){
        stateEntryFlag = true;
        pcSerialComStringWrite( "\r\nERROR: Wi-Fi module not detected!\r\n" );
        pcSerialComStringWrite( "It will re-intent automaticaly in 10 seconds...\r\n" );
        delayInit( &reintentsDelay, 10000 );
    }

    // CHECK TRANSITION CONDITIONS ------------------
    if( delayRead(&reintentsDelay) && !resetSended ) {
        pcSerialComStringWrite( "Reseting Wi-Fi module..\r\n" );
        if( wifiModuleStartReset() == WIFI_MODULE_RESET_STARTED ){
            resetSended = true;
        }
    }
    if ( resetSended ) {            
        switch( wifiModuleResetResponse() ) {
            case WIFI_MODULE_RESET_COMPLETE:
                wifiComFsmState = WIFI_STATE_MODULE_WAIT_FOR_AP_CONNECTION;
            break;
            case WIFI_MODULE_NOT_DETECTED:
                wifiComFsmState = WIFI_STATE_MODULE_NOT_DETECTED;
                stateEntryFlag = false;
                resetSended = false;                
            break;
            default:
            break;
        }
    }

    // EXIT ------------------------------------------
    if( wifiComFsmState != WIFI_STATE_MODULE_NOT_DETECTED ){
        stateEntryFlag = false;
        // Code for exit from this state...  

    }
}

// Chequea si esta conectado y tiene IP.
// Si está conectado y tiene IP pasa al estado WIFI_STATE_COMMUNICATIONS_INIT
// Si no está conectado reintenta conectarse cada 10 segundos.
// Si no detecta el módulo vuelve a: 
//     WIFI_STATE_MODULE_NOT_DETECTED.   
static void runStateWifiModuleWaitForAPConnection()
{
    static bool stateEntryFlag = false;
    // ENTRY ----------------------------------------
    if( stateEntryFlag == false ){
        pcSerialComStringWrite( "\r\nWi-Fi module detected.\r\n" );
        
        stateEntryFlag = true;
    }

    // UPDATE OUTPUTS -------------------------------


    // CHECK TRANSITION CONDITIONS ------------------


    // EXIT ------------------------------------------
    if( wifiComFsmState != WIFI_STATE_MODULE_WAIT_FOR_AP_CONNECTION ){
        stateEntryFlag = false;
        // Code for exit from this state...  

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

    }

    // UPDATE OUTPUTS -------------------------------


    // CHECK TRANSITION CONDITIONS ------------------


    // EXIT ------------------------------------------
    if( wifiComFsmState != WIFI_STATE_COMMUNICATION_INIT ){
        stateEntryFlag = false;
        // Code for exit from this state...  

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

    }

    // UPDATE OUTPUTS -------------------------------


    // CHECK TRANSITION CONDITIONS ------------------


    // EXIT ------------------------------------------
    if( wifiComFsmState != WIFI_STATE_COMMUNICATION_UPDATE ){
        stateEntryFlag = false;
        // Code for exit from this state...  

    }
}
