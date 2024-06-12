#ifndef __WIFI__
#define __WIFI__  "__WIFI__"

#include "global.h"

extern char* wifi_ssid;
extern char* wifi_pwd;

extern int wifi_connected;

void wifi_init_sta(void);

void TaskWifi( void * parameter);

#endif
