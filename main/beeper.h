#ifndef __BEEPER__
#define __BEEPER__  "__BEEPER__"

#include "global.h"

extern int alarmStatus;//警报状态值

extern int alarmSuspend;//临时人为静默


void SET_BEEP(int t);

void SET_ALARM_ON();
void SET_ALARM_OFF();

void SET_ALARM_PROBE_ERROR();
void SET_ALARM_PROBE_OK();

void SET_ALARM_HIGH_ON();

void SET_ALARM_HIGH_OFF();

void SET_ALARM_LOW_ON();

void SET_ALARM_LOW_OFF();

void TaskBeeperCode( void * parameter);

#endif
