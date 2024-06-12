#include "http.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"
#include "esp_http_client.h"
#include <cJSON.h>
#include "nvs_flash.h"

#include "esp_log.h"

#include "global.h"

#include "mqtt.h"


#include "wifi.h"

#define TAG "HTTP"

#define MAX_HTTP_OUTPUT_BUFFER 2048

//这里填写平台分配给您的 产测账号、密码
char* facId = "585659";
char* facSecret = "abc123abc";
// char* facId = "";
// char* facSecret = "";
//一定要有产测账号，才能自动获取到MQTT授权码，请向我们索取！微信: napshen （申工）
//上面写在里面的产测账号的密码已经改掉了（之前不小心上次到了github），请另外向我们索取

//事件回调
static esp_err_t http_event_handle(esp_http_client_event_t *evt)
{
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR://错误事件
            ESP_LOGI(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED://连接成功事件
            ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT://发送头事件
            ESP_LOGI(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER://接收头事件
            ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADER");
            printf("%.*s", evt->data_len, (char*)evt->data);
            break;
        case HTTP_EVENT_ON_DATA://接收数据事件
            ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            
            if (!esp_http_client_is_chunked_response(evt->client)) {
                printf("%.*s", evt->data_len, (char*)evt->data);
            }
            
            char *json_mqtt_config = ((char*)evt->data);
                        
            cJSON *json_obj = cJSON_Parse(json_mqtt_config);

            if (json_obj == NULL) {
                const char *error_ptr = cJSON_GetErrorPtr();
                if (error_ptr != NULL) {
                    printf("Error before: %s", error_ptr);
                }
            }
            else{

                cJSON *mqttPwd = cJSON_GetObjectItem(json_obj, "mqttPwd");
                cJSON *mqttUser = cJSON_GetObjectItem(json_obj, "mqttUser");
                cJSON *mqttDeviceId = cJSON_GetObjectItem(json_obj, "mqttDeviceId");

                if (mqttUser != NULL && mqttPwd != NULL && mqttDeviceId != NULL){

                
                    nvs_handle nvs_handler;
                    nvs_open("cfg_storage", NVS_READWRITE, &nvs_handler);
                    nvs_set_str(nvs_handler, "mqtt_username",  mqttUser->valuestring);
                    nvs_set_str(nvs_handler, "mqtt_password", mqttPwd->valuestring);
                    nvs_set_str(nvs_handler, "device_id", mqttDeviceId->valuestring);

                    nvs_commit(nvs_handler);
                    nvs_close(nvs_handler);

                    vTaskDelay(1000 / portTICK_PERIOD_MS);

                    esp_restart();//reset


                }

            }
            
            cJSON_Delete(json_obj);

            break;
        case HTTP_EVENT_ON_FINISH://会话完成事件
            ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED://断开事件
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
    }
    return ESP_OK;
}





void TaskHTTPCode( void * parameter)
{
    esp_err_t err;

    uint64_t chipid = 0;
    char str_chipid[17];
    esp_efuse_mac_get_default((uint8_t*)&chipid);
    sprintf(str_chipid, "%016llX", chipid);
    printf("Chip ID: %s\n", str_chipid);

    char *clientId_prex = "esp32_";
    char *clientId = "";
    char *clientType = "esp32";

    int client_id_len = strlen(clientId_prex) + strlen(str_chipid) + 1;
    clientId = (char *)malloc(client_id_len + 1);

    memset(clientId,0x00,client_id_len + 1);

    strcpy(clientId, clientId_prex);
    strcat(clientId, str_chipid);


    while(1){
        
        vTaskDelay(5000 / portTICK_PERIOD_MS);

        if(wifi_connected > 0){
        
            printf("****************get_mqtt_info***********************Chip ID: %s\n", str_chipid);

            // Create a cJSON Object
            cJSON *jsonSend = cJSON_CreateObject();

            cJSON_AddStringToObject(jsonSend, "facId",facId);
            cJSON_AddStringToObject(jsonSend, "facSecret",facSecret);
            cJSON_AddStringToObject(jsonSend, "clientId",clientId);
            cJSON_AddStringToObject(jsonSend, "clientType", clientType);

            // Create JSON String From cJSON Object
            char *post_data = cJSON_Print(jsonSend);


            esp_http_client_config_t config = {
                .url = "https://iot-saas-admin.newxton.com/api/factory_device_mqtt_account_renew",
                .event_handler = http_event_handle,
                .skip_cert_common_name_check = true,
                // .user_data = local_response_buffer,  
            };
            esp_http_client_handle_t client = esp_http_client_init(&config);

            // POST
            esp_http_client_set_url(client, "https://iot-saas-admin.newxton.com/api/factory_device_mqtt_account_renew");
            esp_http_client_set_method(client, HTTP_METHOD_POST);
            esp_http_client_set_header(client, "Content-Type", "application/json");
            esp_http_client_set_post_field(client, post_data, strlen(post_data));
            err = esp_http_client_perform(client);
            if (err == ESP_OK) {
                ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %d",
                        esp_http_client_get_status_code(client),
                        esp_http_client_get_content_length(client));
            } else {
                ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
            }

            esp_http_client_cleanup(client);

            cJSON_free(post_data);
            cJSON_Delete(jsonSend);

        }
    }

}