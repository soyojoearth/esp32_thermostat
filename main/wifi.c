#include "wifi.h"

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

#include "mqtt.h"

#include "led.h"

wifi_sta_config_t wifi_sta_config;

int wifi_connected = 0;

/* 默认WiFi名称和密码 */
char* wifi_ssid = "tomato";
char* wifi_pwd = "98761234";

/* 宏定义WiFi连接事件标志位、连接失败标志位及智能配网标志位 */
#define WIFI_CONNECTED_BIT  BIT0
#define WIFI_FAIL_BIT       BIT1
#define SMART_CONFIG_BIT    BIT2

static EventGroupHandle_t wifi_event_group_handler;

/* 系统事件循环处理函数 */
void event_handler(void* arg, esp_event_base_t event_base,int32_t event_id, void* event_data)
{
    static int retry_num = 0;           /* 记录wifi重连次数 */
    /* 系统事件为WiFi事件 */
    if (event_base == WIFI_EVENT)
    {
        if(event_id == WIFI_EVENT_STA_START){    /* 事件id为STA开始 */
            esp_wifi_connect();
            printf("esp connect %d times. \n",retry_num);

        }
        else if (event_id == WIFI_EVENT_STA_CONNECTED) /* connected */
        {
            wifi_connected = 1;
        }
        else if (event_id == WIFI_EVENT_STA_DISCONNECTED) /* 事件id为失去STA连接 */
        {
            wifi_connected = 0;

        	setLedStatusConnecting();//正在连接，状态灯

            esp_wifi_connect();
            retry_num++;
            printf("retry to connect %d times. \n",retry_num);

            if (retry_num > 10)  /* WiFi重连次数大于10 */
            {
                /* 将WiFi连接事件标志组的WiFi连接失败事件位置1 */
                xEventGroupSetBits(wifi_event_group_handler, WIFI_FAIL_BIT);
            }
            /* 清除WiFi连接成功标志位 */
            xEventGroupClearBits(wifi_event_group_handler, WIFI_CONNECTED_BIT);
        }
    }
    /* 系统事件为ip地址事件，且事件id为成功获取ip地址 */
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
    	//系统开机后Wifi连接成功的次数，累加
    	countWifiConnected++;

        wifi_connected = 1;

        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data; /* 获取IP地址信息*/
        printf("got ip:%d.%d.%d.%d \n" , IP2STR(&event->ip_info.ip));  /* 打印ip地址*/
        retry_num = 0;                                              /* WiFi重连次数清零 */
        /* 将WiFi连接事件标志组的WiFi连接成功事件位置1 */
        xEventGroupSetBits(wifi_event_group_handler, WIFI_CONNECTED_BIT);
    }

}

void wifi_init_sta(void)
{

    /* 创建一个事件标志组 */
    wifi_event_group_handler = xEventGroupCreate();

    /* 初始化底层TCP/IP堆栈。在应用程序启动时，应该调用此函数一次。*/
    ESP_ERROR_CHECK(esp_netif_init());

    /* 创建默认事件循环,*/
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* 创建一个默认的WIFI-STA网络接口，如果初始化错误，此API将中止。*/
    esp_netif_create_default_wifi_sta();

    /* 使用WIFI_INIT_CONFIG_DEFAULT() 来获取一个默认的wifi配置参数结构体变量*/
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    /* 根据cfg参数初始化wifi连接所需要的资源 */
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	/* 将事件处理程序注册到系统默认事件循环，分别是WiFi事件、IP地址事件及smartconfig事件 */
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(SC_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));

    memcpy(wifi_sta_config.ssid, wifi_ssid, strlen(wifi_ssid));
    memcpy(wifi_sta_config.password, wifi_pwd, strlen(wifi_pwd));

    wifi_config_t wifi_config;
    wifi_config.sta = wifi_sta_config;

//    wifi_config_t wifi_config = {
//        .sta = {
//            .ssid     = ssid,
//            .password = password,
//        },
//    };

    /* 设置WiFi的工作模式为 STA */
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    /* 设置WiFi连接的参数，主要是ssid和password */
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    /* 启动WiFi连接 */
    ESP_ERROR_CHECK(esp_wifi_start());

    /* 使用事件标志组等待连接建立(WIFI_CONNECTED_BIT)或连接失败(WIFI_FAIL_BIT)事件 */
    EventBits_t bits;  /* 定义一个事件位变量来接收事件标志组等待函数的返回值 */
    bits = xEventGroupWaitBits( wifi_event_group_handler,	        /* 需要等待的事件标志组的句柄 */
                                WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,	/* 需要等待的事件位 */
                                pdFALSE,    /* 为pdFALSE时，在退出此函数之前所设置的这些事件位不变，为pdFALSE则清零*/
                                pdFALSE,    /* 为pdFALSE时，设置的这些事件位任意一个置1就会返回，为pdFALSE则需全为1才返回 */
                                portMAX_DELAY);	                    /* 设置为最长阻塞等待时间，单位为时钟节拍 */

    /* 根据事件标志组等待函数的返回值获取WiFi连接状态 */
    if (bits & WIFI_CONNECTED_BIT)  /* WiFi连接成功事件 */
	{
        printf("connected to ap SSID:%s OK \n",wifi_ssid);
        vEventGroupDelete(wifi_event_group_handler);    /* 删除WiFi连接事件标志组，WiFi连接成功后不再需要 */
    }

}

void TaskWifi( void * parameter)
{
    int mqtt_app_started = 0;
    if(wifiMode == 0){
		setLedStatusConnecting();//状态灯
		wifi_init_sta();
    }

    while (1)
    {
        if(wifi_connected == 1 && mqtt_app_started == 0 && strlen(mqtt_username) > 1){
            mqtt_app_started = 1;
            mqtt_app_start();
        }
        else{
            printf("*************mqtt  started***************\n");
        }
        /* code */
        vTaskDelay(1000 / portTICK_PERIOD_MS);   /* 延时1000ms*/
    }
    
}
