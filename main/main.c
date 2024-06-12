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
#include "wifi.h"
#include "ble.h"

#include "http.h"

#include "aht10.h"

TaskHandle_t TaskBtnCode_Handler;
TaskHandle_t TaskTempCode_Handler;
TaskHandle_t TaskLedStatusCode_Handler;
TaskHandle_t TaskRelayCode_Handler;
TaskHandle_t TaskBeeperCode_Handler;

TaskHandle_t TaskMQTT_Handler;

TaskHandle_t TaskBle_Handler;
TaskHandle_t TaskWifi_Handler;

TaskHandle_t TaskHttp_Handler;

TaskHandle_t TaskAHT10_Handler;

char* json_config;
char* json_mqtt_config;


void app_main(void)
{
    /* 初始化非易失性存储库 (NVS) */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    //加载配置

    nvs_handle cfg_nvs_handler;

    nvs_open("cfg_storage", NVS_READWRITE, &cfg_nvs_handler);
    nvs_get_i32(cfg_nvs_handler,"wifiMode",&wifiMode);//读取失败，wifiMode的值不会改变
    nvs_get_i32(cfg_nvs_handler,"tempUnit",&tempUnit);//读取失败，tempUnit的值不会改变
    nvs_get_i32(cfg_nvs_handler,"tempTarget",&tempTarget);//读取失败，tempTarget的值不会改变
    nvs_get_i32(cfg_nvs_handler,"tempDiff",&tempDiff);//读取失败，tempDiff的值不会改变
    nvs_get_i32(cfg_nvs_handler,"tempCal",&tempCal);//读取失败，tempCal的值不会改变
    nvs_get_i32(cfg_nvs_handler,"tempAlarmH",&tempAlarmH);//读取失败，tempAlarmH的值不会改变
    nvs_get_i32(cfg_nvs_handler,"tempAlarmL",&tempAlarmL);//读取失败，tempAlarmL的值不会改变



    size_t mqtt_username_size = 0;
    size_t mqtt_password_size = 0;
    size_t device_id_size = 0;

    ret = nvs_get_str(cfg_nvs_handler, "device_id", NULL, &device_id_size);
    if (ret == ESP_OK && device_id_size > 0) {
        device_id = malloc(device_id_size);
        ret = nvs_get_str(cfg_nvs_handler, "device_id", device_id, &device_id_size);
    }


    ret = nvs_get_str(cfg_nvs_handler, "mqtt_username", NULL, &mqtt_username_size);
    if (ret == ESP_OK && mqtt_username_size > 0) {
        mqtt_username = malloc(mqtt_username_size);
        ret = nvs_get_str(cfg_nvs_handler, "mqtt_username", mqtt_username, &mqtt_username_size);
    }


    ret = nvs_get_str(cfg_nvs_handler, "mqtt_password", NULL, &mqtt_password_size);
    if (ret == ESP_OK && mqtt_password_size > 0) {
        mqtt_password = malloc(mqtt_password_size);
        ret = nvs_get_str(cfg_nvs_handler, "mqtt_password", mqtt_password, &mqtt_password_size);
    }

    size_t owner_size = 0;
    size_t identifier_size = 0;
    size_t wifiName_size = 0;
    size_t wifiPwd_size = 0;

    ret = nvs_get_str(cfg_nvs_handler, "owner", NULL, &owner_size);
    if (ret == ESP_OK && owner_size > 0) {
        owner = malloc(owner_size);
        ret = nvs_get_str(cfg_nvs_handler, "owner", owner, &owner_size);
    }

    ret = nvs_get_str(cfg_nvs_handler, "identifier", NULL, &identifier_size);
    if (ret == ESP_OK && identifier_size > 0) {
        identifier = malloc(identifier_size);
        ret = nvs_get_str(cfg_nvs_handler, "identifier", identifier, &identifier_size);
    }

    ret = nvs_get_str(cfg_nvs_handler, "wifiName", NULL, &wifiName_size);
    if (ret == ESP_OK && wifiName_size > 0) {
        wifi_ssid = malloc(wifiName_size);
        ret = nvs_get_str(cfg_nvs_handler, "wifiName", wifi_ssid, &wifiName_size);
    }

    ret = nvs_get_str(cfg_nvs_handler, "wifiPwd", NULL, &wifiPwd_size);
    if (ret == ESP_OK && wifiPwd_size > 0) {
        wifi_pwd = malloc(wifiPwd_size);
        ret = nvs_get_str(cfg_nvs_handler, "wifiPwd", wifi_pwd, &wifiPwd_size);
    }


    nvs_close(cfg_nvs_handler);


    initIO();//初始化io口


    if(tempUnit == 1){//华氏度，蓝灯
      led_F();
    }
    else{//摄氏度，黄灯
      led_C();
    }

    TM1640_Init();//初始化数码管    

    xTaskCreatePinnedToCore((TaskFunction_t )TaskBtnCode,          /* 任务函数 */
                                (const char*    )"TaskBtnCode",        /* 任务名称*/
                                (uint16_t       )10000,              /* 任务堆栈大小，单位为字节*/
                                (void*          )NULL,              /* 传递给任务函数的参数*/
                                (UBaseType_t    )20,                /* 任务优先级,最高优先级为24 */
                                (TaskHandle_t*  )&TaskBtnCode_Handler, /* 任务句柄*/
                                (const BaseType_t)tskNO_AFFINITY);  /* 指定运行任务的CPU,使用这个宏表示不会固定到任何核上*/

    xTaskCreatePinnedToCore((TaskFunction_t )TaskTempCode,          /* 任务函数 */
                                (const char*    )"TaskTempCode",        /* 任务名称*/
                                (uint16_t       )10000,              /* 任务堆栈大小，单位为字节*/
                                (void*          )NULL,              /* 传递给任务函数的参数*/
                                (UBaseType_t    )20,                /* 任务优先级,最高优先级为24 */
                                (TaskHandle_t*  )&TaskTempCode_Handler, /* 任务句柄*/
                                (const BaseType_t)tskNO_AFFINITY);  /* 指定运行任务的CPU,使用这个宏表示不会固定到任何核上*/

    xTaskCreatePinnedToCore((TaskFunction_t )TaskLedStatusCode,          /* 任务函数 */
                                (const char*    )"TaskLedStatusCode",        /* 任务名称*/
                                (uint16_t       )5000,              /* 任务堆栈大小，单位为字节*/
                                (void*          )NULL,              /* 传递给任务函数的参数*/
                                (UBaseType_t    )20,                /* 任务优先级,最高优先级为24 */
                                (TaskHandle_t*  )&TaskLedStatusCode_Handler, /* 任务句柄*/
                                (const BaseType_t)tskNO_AFFINITY);  /* 指定运行任务的CPU,使用这个宏表示不会固定到任何核上*/

    xTaskCreatePinnedToCore((TaskFunction_t )TaskRelayCode,          /* 任务函数 */
                                (const char*    )"TaskRelayCode",        /* 任务名称*/
                                (uint16_t       )10000,              /* 任务堆栈大小，单位为字节*/
                                (void*          )NULL,              /* 传递给任务函数的参数*/
                                (UBaseType_t    )20,                /* 任务优先级,最高优先级为24 */
                                (TaskHandle_t*  )&TaskRelayCode_Handler, /* 任务句柄*/
                                (const BaseType_t)tskNO_AFFINITY);  /* 指定运行任务的CPU,使用这个宏表示不会固定到任何核上*/

    xTaskCreatePinnedToCore((TaskFunction_t )TaskBeeperCode,          /* 任务函数 */
                                (const char*    )"TaskBeeperCode",        /* 任务名称*/
                                (uint16_t       )10000,              /* 任务堆栈大小，单位为字节*/
                                (void*          )NULL,              /* 传递给任务函数的参数*/
                                (UBaseType_t    )20,                /* 任务优先级,最高优先级为24 */
                                (TaskHandle_t*  )&TaskBeeperCode_Handler, /* 任务句柄*/
                                (const BaseType_t)tskNO_AFFINITY);  /* 指定运行任务的CPU,使用这个宏表示不会固定到任何核上*/

    xTaskCreatePinnedToCore((TaskFunction_t )TaskMQTTCode,          /* 任务函数 */
                            (const char*    )"TaskMQTTCode",        /* 任务名称*/
                            (uint16_t       )10000,              /* 任务堆栈大小，单位为字节*/
                            (void*          )NULL,              /* 传递给任务函数的参数*/
                            (UBaseType_t    )20,                /* 任务优先级,最高优先级为24 */
                            (TaskHandle_t*  )&TaskMQTT_Handler, /* 任务句柄*/
                            (const BaseType_t)tskNO_AFFINITY);  /* 指定运行任务的CPU,使用这个宏表示不会固定到任何核上*/

    xTaskCreatePinnedToCore((TaskFunction_t )TaskBle,          /* 任务函数 */
                                (const char*    )"TaskBle",        /* 任务名称*/
                                (uint16_t       )10000,              /* 任务堆栈大小，单位为字节*/
                                (void*          )NULL,              /* 传递给任务函数的参数*/
                                (UBaseType_t    )20,                /* 任务优先级,最高优先级为24 */
                                (TaskHandle_t*  )&TaskBle_Handler, /* 任务句柄*/
                                (const BaseType_t)tskNO_AFFINITY);  /* 指定运行任务的CPU,使用这个宏表示不会固定到任何核上*/

    xTaskCreatePinnedToCore((TaskFunction_t )TaskWifi,          /* 任务函数 */
                                (const char*    )"TaskWifi",        /* 任务名称*/
                                (uint16_t       )10000,              /* 任务堆栈大小，单位为字节*/
                                (void*          )NULL,              /* 传递给任务函数的参数*/
                                (UBaseType_t    )20,                /* 任务优先级,最高优先级为24 */
                                (TaskHandle_t*  )&TaskWifi_Handler, /* 任务句柄*/
                                (const BaseType_t)tskNO_AFFINITY);  /* 指定运行任务的CPU,使用这个宏表示不会固定到任何核上*/


    xTaskCreatePinnedToCore((TaskFunction_t )TaskAHT10,          /* 任务函数 */
                                (const char*    )"TaskAHT10",        /* 任务名称*/
                                (uint16_t       )5000,              /* 任务堆栈大小，单位为字节*/
                                (void*          )NULL,              /* 传递给任务函数的参数*/
                                (UBaseType_t    )20,                /* 任务优先级,最高优先级为24 */
                                (TaskHandle_t*  )&TaskAHT10_Handler, /* 任务句柄*/
                                (const BaseType_t)tskNO_AFFINITY);  /* 指定运行任务的CPU,使用这个宏表示不会固定到任何核上*/

    if (
        strlen(device_id)  > 0 
        && strlen(mqtt_username)  > 0
        && strlen(mqtt_password)  > 0
        )
    {
        printf("*************is_get_mqtt_info **************\n");
    }
    else{

        if(strlen(facId)==0 || strlen(facSecret)==0){
            printf("****** FacId and facSecret cannot be found. The device needs facId and facSecret to automatically obtain the authorized access code of the IoT platform. Please request it from us (18823207889@139.com)*****\n");
        }
        else{
            printf("*************prepare_get_mqtt_info **************\n");
            xTaskCreatePinnedToCore((TaskFunction_t )TaskHTTPCode,          /* 任务函数 */
                                    (const char*    )"TaskHTTPCode",        /* 任务名称*/
                                    (uint16_t       )10000,              /* 任务堆栈大小，单位为字节*/
                                    (void*          )NULL,              /* 传递给任务函数的参数*/
                                    (UBaseType_t    )20,                /* 任务优先级,最高优先级为24 */
                                    (TaskHandle_t*  )&TaskHttp_Handler, /* 任务句柄*/
                                    (const BaseType_t)tskNO_AFFINITY);  /* 指定运行任务的CPU,使用这个宏表示不会固定到任何核上*/
        }
        
    }


    while(1){

    	vTaskDelay(1000 / portTICK_PERIOD_MS);   /* 延时1000ms*/
    	runningTime++;//系统开机时间累计，单位：秒
        printf("---------------Main----- heap:%u --------------------------\r\n", esp_get_free_heap_size());

        // printf("owner:%s\n",owner);
        // printf("identifier:%s\n",identifier);

        printf("wifi_ssid:%s\n",wifi_ssid);
        printf("wifi_pwd:%s\n",wifi_pwd);

        printf("borker_host:%s\n",borker_host);

        printf("topic_up:%s\n",topic_up);
        printf("topic_down:%s\n",topic_down);

        printf("device_id:%s\n",device_id);
        printf("mqtt_username:%s\n",mqtt_username);
        printf("mqtt_password:%s\n",mqtt_password);

        // uint64_t chipid = 0;
        // char str_chipid[17];
        // esp_efuse_mac_get_default((uint8_t*)&chipid);
        // sprintf(str_chipid, "%016llX", chipid);
        // printf("Chip ID: %s\n", str_chipid);

        if(strlen(facId)==0 || strlen(facSecret)==0){
            printf("****** FacId and facSecret cannot be found. The device needs facId and facSecret to automatically obtain the authorized access code of the IoT platform. Please request it from us (18823207889@139.com)*****\n");
        }

    }

}