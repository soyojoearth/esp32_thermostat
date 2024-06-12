#ifndef __MQTT__
#define __MQTT__  "__MQTT__"

#include "global.h"


extern char *device_id;
extern char *mqtt_username;
extern char *mqtt_password;

extern char *topic_up;
extern char *topic_down;

extern char *owner;
extern char *identifier;
/**这些内容是可以被App配网覆盖的**/
extern char *borker_host;
/**这些内容是可以被App配网覆盖的**/


extern int mqtt_upload_atonce;

void mqtt_app_start(void);

void unpairDevice();

//每隔15秒上报一次消息
void TaskMQTTCode( void * parameter);

#endif
