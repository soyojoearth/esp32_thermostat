#ifndef __AHT10__
#define __AHT10__  "__AHT10__"

#include "global.h"


extern int32_t tempAht;
extern int32_t humAht;

extern bool ahtStatusOK;


void TaskAHT10(void *arg);

#endif
