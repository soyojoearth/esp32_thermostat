#ifndef __TEMP__
#define __TEMP__  "__TEMP__"

#include "global.h"

extern int currentTempC;
extern int currentTempF;

void TaskTempCode( void * parameter);

int convertCtoF10(int temp);

int convertFtoC10(int temp);

void convertSettingsFtoC();

void convertSettingsCtoF();

void updateCurrentToLCD();

void showCurrentSettingLCD();

#endif
