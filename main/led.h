#ifndef __LED__
#define __LED__  "__LED__"

extern int led_mode;

void led_C();
void led_F();

void setLedStatusConnecting();
void setLedStatusConnected();
void setLedStatusBluetooth();
void setLedStatusWiFiHot();

void setLedOnOffTime(int t_on,int t_off);

void TaskLedStatusCode( void * parameter);

#endif
