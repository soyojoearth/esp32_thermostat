#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_wifi.h"
#include "driver/gpio.h"
#include "nvs_flash.h"
#include "esp_smartconfig.h"

#include <sys/param.h>
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "mqtt_client.h"
#include <cJSON.h>

#include "global.h"

#include "led.h"
#include "beeper.h"

#include "temp.h"
#include "relay.h"

#include "btn.h"


#include "tm1640.h"

#include "mqtt.h"

#include "http.h"

char *topic_up_prex = "device/up/";
char *topic_down_prex = "device/down/";

/**这些内容是默认值**/
char *product_id = "jdh36g92s71";

char *borker_host = "iot-mqtt-host-tls.newxton.com";
/**以上这些内容是默认值**/

/**这些内容是出厂时写入的默认值**/
char *device_id="";
char *topic_up="";
char *topic_down="";
char *mqtt_username="";
char *mqtt_password="";
/**以上这些内容是出厂时写入的默认值**/

/**这些内容是可以被App配网覆盖的**/
char *owner = "";
char *identifier = "";
/**这些内容是可以被App配网覆盖的**/

int mqtt_port = 8883;


const int subQoS = 1;     // 客户端发布主题时使用的QoS级别
const bool cleanSession = true; // 清除会话（如QoS>0必须要设为false）
const char* willMsg = "{\"netStatus\":\"offline\"}";     // 遗嘱主题信息
const int willQos = 1;               // 遗嘱QoS
const int willRetain = 0;        // 遗嘱保留


int mqtt_isconnected = 0;

int mqtt_upload_atonce = 0;

char mqtt_upload_buffer[255];


char mqtt_receive_buffer[255];


esp_mqtt_client_handle_t client;

void parseReceiveData(char *data);

void sendPairData();

void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);

uint16_t CRC16_CCITT_FALSE(char *data, uint16_t len);

void mqtt_app_start()
{

	int topic_up_len = strlen(topic_up_prex) + strlen(device_id) + 1; // 计算拼接后的字符串长度（加1是为了空出字符串末尾的'\0'）
	topic_up = (char *)malloc(topic_up_len + 1); // 为拼接后的字符串分配内存空间（加1也是为了空出字符串末尾的'\0'）

	memset(topic_up,0x00,topic_up_len + 1);

	strcpy(topic_up, topic_up_prex); // 将topic_up_prex拷贝到topic_up中
	strcat(topic_up, device_id); // 将device_id拼接到topic_up后面

	int topic_down_len = strlen(topic_down_prex) + strlen(device_id) + 1;
	topic_down = (char *)malloc(topic_down_len + 1); 

	memset(topic_down,0x00,topic_down_len + 1);

	strcpy(topic_down, topic_down_prex); 
	strcat(topic_down, device_id);

    // 1、定义一个MQTT客户端配置结构体，输入MQTT的url
    esp_mqtt_client_config_t mqtt_cfg = {
    	.host = borker_host,
        .username = mqtt_username,
		.password = mqtt_password,
		.port = mqtt_port,
		.keepalive = 60,
		.disable_clean_session = false,
		.lwt_topic = topic_up,
		.lwt_msg = willMsg,
		.lwt_qos = willQos,
		.lwt_retain = willRetain,
		.lwt_msg_len = strlen(willMsg),
		.transport = MQTT_TRANSPORT_OVER_SSL,
		.protocol_ver = MQTT_PROTOCOL_V_3_1_1,
		.skip_cert_common_name_check = true
    };


    // 2、通过esp_mqtt_client_init获取一个MQTT客户端结构体指针，参数是MQTT客户端配置结构体
    client = esp_mqtt_client_init(&mqtt_cfg);

    // 3、注册MQTT事件
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);

    // 4、开启MQTT功能
    esp_mqtt_client_start(client);
}

// 事件处理函数
void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    //printf("Event dispatched from event loop base=%s, event_id=%d \n", base, event_id);
    // 获取MQTT客户端结构体指针
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    // 通过事件ID来分别处理对应的事件
    switch (event->event_id)
    {
        // 建立连接成功
        case MQTT_EVENT_CONNECTED:
        	mqtt_isconnected = 1;
        	setLedStatusConnected();
            printf("MQTT_client cnnnect success. \n");
            // 订阅主题，qos=0
            esp_mqtt_client_subscribe(client, topic_down, 0);

            //发送第一次消息
            sendPairData();

            //系统开机后Mqtt连接成功的次数，累加
            countMqttConnected++;

			mqtt_upload_atonce = 1; //上报消息

            break;
        // 客户端断开连接
        case MQTT_EVENT_DISCONNECTED:
        	mqtt_isconnected = 0;
            printf("MQTT_client have disconnected. \n");
            setLedStatusConnecting();//正在连接，状态灯
            break;
        // 主题订阅成功
        case MQTT_EVENT_SUBSCRIBED:
            printf("mqtt subscribe ok. msg_id = %d \n",event->msg_id);
            break;
        // 取消订阅
        case MQTT_EVENT_UNSUBSCRIBED:
            printf("mqtt unsubscribe ok. msg_id = %d \n",event->msg_id);
            break;
        //  主题发布成功
        case MQTT_EVENT_PUBLISHED:
            printf("mqtt published ok. msg_id = %d \n",event->msg_id);
            break;
        // 已收到订阅的主题消息
        case MQTT_EVENT_DATA:
            printf("mqtt received topic: %.*s \n",event->topic_len, event->topic);
		
			//Nxt Protocol D-TLV

			printf("message hex: ");
			for (size_t i = 0; i < event->data_len; i++)
			{
				printf("%02x ",event->data[i]);
				mqtt_receive_buffer[i] = event->data[i];
			}
			printf(" \n");

			parseReceiveData(mqtt_receive_buffer);          

            break;
        // 客户端遇到错误
        case MQTT_EVENT_ERROR:
        	mqtt_isconnected = 0;
        	setLedStatusConnecting();//正在连接，状态灯
            printf("MQTT_EVENT_ERROR \n");
            break;
        default:
            printf("Other event id:%d \n", event->event_id);
            break;
    }
}

void random_string(char *str, size_t length)
{
    const char charset[] = "0123456789abcdefghijklmnopqrstuvwxyz";
    if (length) {
        --length;
        for (size_t n = 0; n < length; n++) {
            int key = esp_random() % (int) (sizeof charset - 1);
            str[n] = charset[key];
        }
        str[length] = '\0';
    }
}

void sendPairData(){
	if(wifiMode == 0){
		if (mqtt_isconnected){//检查是否已经连接

			char buffer[8];
			random_string(buffer, sizeof(buffer));//随机数字字符串

			// 创建一个cJSON对象
			cJSON *jsonSend = cJSON_CreateObject();

			if(strlen(owner) == 0){
				cJSON_AddNullToObject(jsonSend, "owner");
			}
			else{
				cJSON_AddStringToObject(jsonSend, "owner",owner);
			}

			if(strlen(identifier) == 0){
				cJSON_AddNullToObject(jsonSend, "identifier");
			}
			else{
				cJSON_AddStringToObject(jsonSend, "identifier", identifier);
			}

			cJSON_AddStringToObject(jsonSend, "productId", product_id);

			// 使用cJSON_Print生成JSON字符串
			char *json_string_s = cJSON_Print(jsonSend);
			esp_mqtt_client_publish(client, topic_up, json_string_s, 0, 2, 0);

			cJSON_free(json_string_s);
			cJSON_Delete(jsonSend);

			//设备解绑，identifier的值要和原先配对码不一样

		}
	}
}

void unpairDevice(){

	//设备解绑，identifier的值要和原先配对码不一样
	//update identifier to rand str
	char buffer[16];
	random_string(buffer, sizeof(buffer));//随机数字字符串
	identifier = malloc(sizeof(buffer)); // 分配内存
	strcpy(identifier, buffer);

	//clear owner
	if(strlen(owner)>0){
		strcpy(owner, "");
	}

	// 打开 NVS 存储空间
	nvs_handle nvs_handler;
	nvs_open("cfg_storage", NVS_READWRITE, &nvs_handler);
	nvs_erase_key(nvs_handler, "wifiName");//delete wifiName
	nvs_erase_key(nvs_handler, "wifiPwd");//delete wifiPwd
	nvs_erase_key(nvs_handler, "owner");//delete owner
	nvs_set_str(nvs_handler, "identifier", identifier);
	nvs_commit(nvs_handler);
	nvs_close(nvs_handler);

	if (mqtt_isconnected){//检查是否已经连接
		sendPairData();
	}

}



uint16_t CRC16_CCITT_FALSE(char *data, uint16_t len)  
{  
    uint16_t wCRCin = 0xFFFF;  
    uint16_t wCPoly = 0x1021;  
    char wChar = 0;  
    
    while (len--)
    {  
        wChar = *(data++);  
        wCRCin ^= (wChar << 8); 
        
        for(int i = 0; i < 8; i++)  
        {  
            if(wCRCin & 0x8000)  
            {
                wCRCin = (wCRCin << 1) ^ wCPoly;  
            }
            else  
            {
                wCRCin = wCRCin << 1; 
            }            
        }  
    }  
    return (wCRCin) ;  
}



void parseReceiveData(char *data){	

	if (data[0] == (uint8_t)0xAA && data[1] == (uint8_t)0x55)
	{
		printf("Nxt Protocol D-TLV \n");
		//Nxt Protocol D-TLV
		uint16_t length = (data[3] & 0x00ff)<<8 | (data[4] & 0x00ff);

		printf("Nxt Protocol length %d \n",length);
		//CRC
		uint16_t length_crc = length + 2 + 1;
		uint16_t crc_index = length + 2 + 1 + 2;
		uint16_t crc_data = (data[crc_index] & 0x00ff)<<8 | (data[crc_index+1] & 0x00ff);

		printf("Nxt Protocol length_crc:%d \n",length_crc);

		uint16_t crc_result = CRC16_CCITT_FALSE(&(data[2]), length_crc);
		uint8_t crc_result_1 = (crc_result >> 8) & 0xff;
		uint8_t crc_result_2 = crc_result & 0xff;

		printf("crc %02x %02x \n",crc_result_1,crc_result_2);

		printf("Nxt Protocol 3 \n");
		if (crc_result != crc_data)
		{
			printf("crc error %02x  %02x \n",crc_result_1,crc_result_2);
		}
		else{
			printf("crc ok %02x  %02x \n",crc_result_1,crc_result_2);

			int last_payload_length = length;
			int index_payload = 5;

			uint8_t item_dpid;
			uint8_t item_type;
			uint16_t item_len;
			uint16_t index_value;

			while (last_payload_length >= 4)
			{
				item_dpid = data[index_payload + 0];
				item_type = data[index_payload + 1];
				item_len = ((data[index_payload + 2] & 0x00ff) << 8) | (data[index_payload + 3] & 0x00ff);
				index_value = index_payload + 4;

				if(item_dpid == 106){
					//tempUnit

					uint8_t dpValue = data[index_value];

					if(dpValue == 0 && tempUnit == 1){
						tempUnit = 0;
						convertSettingsFtoC();
					}
					if(dpValue == 1 && tempUnit == 0){
						tempUnit = 1;
						convertSettingsCtoF();
					}

				}
				else if(item_dpid == 101){
					//tempTarget
					int32_t dpValue = ((data[index_value] & 0xffffffff) << 24) | 
					((data[index_value+1] & 0xffffffff) << 16) | 
					((data[index_value+2] & 0xffffffff) << 8) | 
					((data[index_value+3] & 0xffffffff) << 0);

					tempTarget = dpValue;

					printf("dpid:%d dpValue:%d\n",item_dpid,dpValue);
				}
				else if(item_dpid == 102){
					//tempDiff
					int32_t dpValue = ((data[index_value] & 0xffffffff) << 24) | 
					((data[index_value+1] & 0xffffffff) << 16) | 
					((data[index_value+2] & 0xffffffff) << 8) | 
					((data[index_value+3] & 0xffffffff) << 0);

					tempDiff = dpValue;

					printf("dpid:%d dpValue:%d\n",item_dpid,dpValue);

				}
				else if(item_dpid == 103){
					//tempCal
					int32_t dpValue = ((data[index_value] & 0xffffffff) << 24) | 
					((data[index_value+1] & 0xffffffff) << 16) | 
					((data[index_value+2] & 0xffffffff) << 8) | 
					((data[index_value+3] & 0xffffffff) << 0);

					tempCal = dpValue;

					printf("dpid:%d dpValue:%d\n",item_dpid,dpValue);

				}
				else if(item_dpid == 104){
					//tempAlarmH
					int32_t dpValue = ((data[index_value] & 0xffffffff) << 24) | 
					((data[index_value+1] & 0xffffffff) << 16) | 
					((data[index_value+2] & 0xffffffff) << 8) | 
					((data[index_value+3] & 0xffffffff) << 0);

					tempAlarmH = dpValue;

					printf("dpid:%d dpValue:%d\n",item_dpid,dpValue);

				}
				else if(item_dpid == 105){
					//tempAlarmL
					int32_t dpValue = ((data[index_value] & 0xffffffff) << 24) | 
					((data[index_value+1] & 0xffffffff) << 16) | 
					((data[index_value+2] & 0xffffffff) << 8) | 
					((data[index_value+3] & 0xffffffff) << 0);

					tempAlarmL = dpValue;

					printf("dpid:%d dpValue:%d\n",item_dpid,dpValue);

				}
				else if(item_dpid == 111){
					//coolingDelay
					int32_t dpValue = ((data[index_value] & 0xffffffff) << 24) | 
					((data[index_value+1] & 0xffffffff) << 16) | 
					((data[index_value+2] & 0xffffffff) << 8) | 
					((data[index_value+3] & 0xffffffff) << 0);

					coolingDelay = dpValue;

					printf("dpid:%d dpValue:%d\n",item_dpid,dpValue);

				}
				else if(item_dpid == 112){
					//heatingDelay
					int32_t dpValue = ((data[index_value] & 0xffffffff) << 24) | 
					((data[index_value+1] & 0xffffffff) << 16) | 
					((data[index_value+2] & 0xffffffff) << 8) | 
					((data[index_value+3] & 0xffffffff) << 0);

					heatingDelay = dpValue;

					printf("dpid:%d dpValue:%d\n",item_dpid,dpValue);

				}
				else if(item_dpid == 78){
					//unbind
					unpairDevice();//unpair wifi & ble
					printf("Nxt Protocol unbind \n");
				}

				last_payload_length = last_payload_length - 4 - item_len;
				index_payload = index_payload + item_len + 4;				

			}
						
			saveSettings();//保存

			mqtt_upload_atonce = 1;//重发一下

		}

	}
	
}

void TaskMQTTCode( void * parameter)
{
	uint16_t index;
  	static int t_mqtt_count = 0;

  while(1){

	vTaskDelay(10 / portTICK_PERIOD_MS);   /* 延时10ms*/

	t_mqtt_count++;

	if(t_mqtt_count >= 3000){// per 30s upload once least
		t_mqtt_count = 0;
		mqtt_upload_atonce = 1;
	}

	if (mqtt_upload_atonce == 0)
	{
		continue;
	}
	
	//检查是否已经连接
	if(!(wifiMode == 0 && mqtt_isconnected)){
		vTaskDelay(1000 / portTICK_PERIOD_MS);   /* 延时10ms*/
		continue;
	}

	mqtt_upload_atonce = 0;

	

	//Protocol Head: 0xAA 0x55
	mqtt_upload_buffer[0] = 0xAA;
	mqtt_upload_buffer[1] = 0x55;

	//Protocol Version
	mqtt_upload_buffer[2] = 0x02;

	index = 5;

	//ProductId
	mqtt_upload_buffer[5] = 0x00;
	mqtt_upload_buffer[6] = 0x01;
	mqtt_upload_buffer[7] = 0x00;
	mqtt_upload_buffer[8] = strlen(product_id) & 0xff;

	index += 4;

	for (size_t i = 0; i < strlen(product_id); i++)
	{
		mqtt_upload_buffer[index+i] = product_id[i];
	}
	
	index += strlen(product_id);

	//currentTempC
	mqtt_upload_buffer[index+0] = 108;
	mqtt_upload_buffer[index+1] = 0x02;//value
	mqtt_upload_buffer[index+2] = 0x00;
	mqtt_upload_buffer[index+3] = 0x04;
	mqtt_upload_buffer[index+4] = (currentTempC >> 24) & 0xff;
	mqtt_upload_buffer[index+5] = (currentTempC >> 16) & 0xff;
	mqtt_upload_buffer[index+6] = (currentTempC >> 8) & 0xff;
	mqtt_upload_buffer[index+7] = (currentTempC >> 0) & 0xff;

	index += 8;

	//currentTempF
	mqtt_upload_buffer[index+0] = 109;
	mqtt_upload_buffer[index+1] = 0x02;//value
	mqtt_upload_buffer[index+2] = 0x00;
	mqtt_upload_buffer[index+3] = 0x04;
	mqtt_upload_buffer[index+4] = (currentTempF >> 24) & 0xff;
	mqtt_upload_buffer[index+5] = (currentTempF >> 16) & 0xff;
	mqtt_upload_buffer[index+6] = (currentTempF >> 8) & 0xff;
	mqtt_upload_buffer[index+7] = (currentTempF >> 0) & 0xff;

	index += 8;

	//tempUnit
	mqtt_upload_buffer[index+0] = 106;
	mqtt_upload_buffer[index+1] = 0x04;//enum
	mqtt_upload_buffer[index+2] = 0x00;
	mqtt_upload_buffer[index+3] = 0x01;
	mqtt_upload_buffer[index+4] = tempUnit & 0xff;

	index += 5;

	//tempTarget
	mqtt_upload_buffer[index+0] = 101;
	mqtt_upload_buffer[index+1] = 0x02;//value
	mqtt_upload_buffer[index+2] = 0x00;
	mqtt_upload_buffer[index+3] = 0x04;
	mqtt_upload_buffer[index+4] = (tempTarget >> 24) & 0xff;
	mqtt_upload_buffer[index+5] = (tempTarget >> 16) & 0xff;
	mqtt_upload_buffer[index+6] = (tempTarget >> 8) & 0xff;
	mqtt_upload_buffer[index+7] = (tempTarget >> 0) & 0xff;

	index += 8;

	//tempDiff
	mqtt_upload_buffer[index+0] = 102;
	mqtt_upload_buffer[index+1] = 0x02;//value
	mqtt_upload_buffer[index+2] = 0x00;
	mqtt_upload_buffer[index+3] = 0x04;
	mqtt_upload_buffer[index+4] = (tempDiff >> 24) & 0xff;
	mqtt_upload_buffer[index+5] = (tempDiff >> 16) & 0xff;
	mqtt_upload_buffer[index+6] = (tempDiff >> 8) & 0xff;
	mqtt_upload_buffer[index+7] = (tempDiff >> 0) & 0xff;

	index += 8;

	//tempCal
	mqtt_upload_buffer[index+0] = 103;
	mqtt_upload_buffer[index+1] = 0x02;//value
	mqtt_upload_buffer[index+2] = 0x00;
	mqtt_upload_buffer[index+3] = 0x04;
	mqtt_upload_buffer[index+4] = (tempCal >> 24) & 0xff;
	mqtt_upload_buffer[index+5] = (tempCal >> 16) & 0xff;
	mqtt_upload_buffer[index+6] = (tempCal >> 8) & 0xff;
	mqtt_upload_buffer[index+7] = (tempCal >> 0) & 0xff;

	index += 8;

	//tempAlarmH
	mqtt_upload_buffer[index+0] = 104;
	mqtt_upload_buffer[index+1] = 0x02;//value
	mqtt_upload_buffer[index+2] = 0x00;
	mqtt_upload_buffer[index+3] = 0x04;
	mqtt_upload_buffer[index+4] = (tempAlarmH >> 24) & 0xff;
	mqtt_upload_buffer[index+5] = (tempAlarmH >> 16) & 0xff;
	mqtt_upload_buffer[index+6] = (tempAlarmH >> 8) & 0xff;
	mqtt_upload_buffer[index+7] = (tempAlarmH >> 0) & 0xff;

	index += 8;

	//tempAlarmL
	mqtt_upload_buffer[index+0] = 105;
	mqtt_upload_buffer[index+1] = 0x02;//value
	mqtt_upload_buffer[index+2] = 0x00;
	mqtt_upload_buffer[index+3] = 0x04;
	mqtt_upload_buffer[index+4] = (tempAlarmL >> 24) & 0xff;
	mqtt_upload_buffer[index+5] = (tempAlarmL >> 16) & 0xff;
	mqtt_upload_buffer[index+6] = (tempAlarmL >> 8) & 0xff;
	mqtt_upload_buffer[index+7] = (tempAlarmL >> 0) & 0xff;

	index += 8;

	//coolingDelay
	mqtt_upload_buffer[index+0] = 111;
	mqtt_upload_buffer[index+1] = 0x02;//value
	mqtt_upload_buffer[index+2] = 0x00;
	mqtt_upload_buffer[index+3] = 0x04;
	mqtt_upload_buffer[index+4] = (coolingDelay >> 24) & 0xff;
	mqtt_upload_buffer[index+5] = (coolingDelay >> 16) & 0xff;
	mqtt_upload_buffer[index+6] = (coolingDelay >> 8) & 0xff;
	mqtt_upload_buffer[index+7] = (coolingDelay >> 0) & 0xff;

	index += 8;

	//heatingDelay
	mqtt_upload_buffer[index+0] = 112;
	mqtt_upload_buffer[index+1] = 0x02;//value
	mqtt_upload_buffer[index+2] = 0x00;
	mqtt_upload_buffer[index+3] = 0x04;
	mqtt_upload_buffer[index+4] = (heatingDelay >> 24) & 0xff;
	mqtt_upload_buffer[index+5] = (heatingDelay >> 16) & 0xff;
	mqtt_upload_buffer[index+6] = (heatingDelay >> 8) & 0xff;
	mqtt_upload_buffer[index+7] = (heatingDelay >> 0) & 0xff;

	index += 8;

	//alarmStatus
	mqtt_upload_buffer[index+0] = 121;
	mqtt_upload_buffer[index+1] = 0x05;//bitmap
	mqtt_upload_buffer[index+2] = 0x00;
	mqtt_upload_buffer[index+3] = 0x04;
	mqtt_upload_buffer[index+4] = (alarmStatus >> 24) & 0xff;
	mqtt_upload_buffer[index+5] = (alarmStatus >> 16) & 0xff;
	mqtt_upload_buffer[index+6] = (alarmStatus >> 8) & 0xff;
	mqtt_upload_buffer[index+7] = (alarmStatus >> 0) & 0xff;

	index += 8;


	//Heating
	mqtt_upload_buffer[index+0] = 131;
	mqtt_upload_buffer[index+1] = 0x01;//bool
	mqtt_upload_buffer[index+2] = 0x00;
	mqtt_upload_buffer[index+3] = 0x01;
	mqtt_upload_buffer[index+4] = isHeating ? 0x01 : 0x00;

	index += 5;

	//Cooling
	mqtt_upload_buffer[index+0] = 132;
	mqtt_upload_buffer[index+1] = 0x01;//bool
	mqtt_upload_buffer[index+2] = 0x00;
	mqtt_upload_buffer[index+3] = 0x01;
	mqtt_upload_buffer[index+4] = isCooling ? 0x01 : 0x00;

	index += 5;

	//runningTime
	mqtt_upload_buffer[index+0] = 150;
	mqtt_upload_buffer[index+1] = 0x02;//value
	mqtt_upload_buffer[index+2] = 0x00;
	mqtt_upload_buffer[index+3] = 0x04;
	mqtt_upload_buffer[index+4] = (runningTime >> 24) & 0xff;
	mqtt_upload_buffer[index+5] = (runningTime >> 16) & 0xff;
	mqtt_upload_buffer[index+6] = (runningTime >> 8) & 0xff;
	mqtt_upload_buffer[index+7] = (runningTime >> 0) & 0xff;

	index += 8;

	//countWifiConnected
	mqtt_upload_buffer[index+0] = 151;
	mqtt_upload_buffer[index+1] = 0x02;//value
	mqtt_upload_buffer[index+2] = 0x00;
	mqtt_upload_buffer[index+3] = 0x04;
	mqtt_upload_buffer[index+4] = (countWifiConnected >> 24) & 0xff;
	mqtt_upload_buffer[index+5] = (countWifiConnected >> 16) & 0xff;
	mqtt_upload_buffer[index+6] = (countWifiConnected >> 8) & 0xff;
	mqtt_upload_buffer[index+7] = (countWifiConnected >> 0) & 0xff;

	index += 8;


	//countMqttConnected
	mqtt_upload_buffer[index+0] = 152;
	mqtt_upload_buffer[index+1] = 0x02;//value
	mqtt_upload_buffer[index+2] = 0x00;
	mqtt_upload_buffer[index+3] = 0x04;
	mqtt_upload_buffer[index+4] = (countMqttConnected >> 24) & 0xff;
	mqtt_upload_buffer[index+5] = (countMqttConnected >> 16) & 0xff;
	mqtt_upload_buffer[index+6] = (countMqttConnected >> 8) & 0xff;
	mqtt_upload_buffer[index+7] = (countMqttConnected >> 0) & 0xff;

	index += 8;


	//identifier
	mqtt_upload_buffer[index+0] = 153;
	mqtt_upload_buffer[index+1] = 0x03;
	mqtt_upload_buffer[index+2] = 0x00;
	mqtt_upload_buffer[index+3] = strlen(identifier) & 0xff;

	index += 4;

	for (size_t i = 0; i < strlen(identifier); i++)
	{
		mqtt_upload_buffer[index+i] = identifier[i];
	}
	
	index += strlen(identifier);


	//payload length
	uint16_t payload_length = index-5;

	mqtt_upload_buffer[3] = (payload_length >> 8) & 0xff;
	mqtt_upload_buffer[4] = (payload_length >> 0) & 0xff;

	//CRC
	uint16_t crc_result = CRC16_CCITT_FALSE(&mqtt_upload_buffer[2], payload_length+3);
	uint8_t crc_result_1 = (crc_result >> 8) & 0xff;
	uint8_t crc_result_2 = crc_result & 0xff;

	mqtt_upload_buffer[payload_length+5+0] = crc_result_1;
	mqtt_upload_buffer[payload_length+5+1] = crc_result_2;

	index += 2;

	uint16_t length_send = index;

	//MQTT
	esp_mqtt_client_publish(client, topic_up, mqtt_upload_buffer, length_send, 0, 0);

	printf("upload hex: ");
	for (size_t i = 0; i < length_send; i++)
	{
		printf("%02x ",mqtt_upload_buffer[i]);
	}

	printf(" \n");
	

  }

}