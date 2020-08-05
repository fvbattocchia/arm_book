//=====[#include guards - begin]===============================================

#ifndef _FIRE_ALARM_H_
#define _FIRE_ALARM_H_

//=====[Libraries]=============================================================

//=====[Declaration of public constants]=======================================



//=====[Declaration of public data types]======================================

//=====[Declarations (prototypes) of public functions]=========================

void fireAlarmInit();
void fireAlarmUpdate();
bool gasDetectorStateRead();
bool overTempDetectorStateRead();
bool gasDetectedRead();
bool overTempDetectedRead();

//=====[#include guards - end]=================================================

#endif // _FIRE_ALARM_H_