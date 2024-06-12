#ifndef __TM1640__
#define __TM1640__  "__TM1640__"

#include "global.h"

void TM1640_Init(void); //TM1640初始化函数

void TM1640_ShowTempError(void);

void TM1640_ShowTemp(signed short value);//显示温度，温度放大10倍

void TM1640_ShowTarget(signed short value);//显示目标温度，前面加个P

void TM1640_ShowDiff(signed short value);//显示回差值.

void TM1640_ShowCal(signed short value);//显示修正值

void TM1640_ShowAlarmHigh(signed short value);//显示高温警报值

void TM1640_ShowAlarmLow(signed short value);//显示低温警报值

#endif
