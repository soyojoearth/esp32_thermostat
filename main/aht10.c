
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "driver/i2c.h"

#include "temp.h"
 
 
#define slave_addr 0x38
#define scl_gpio 22
#define sda_gpio 21
#define commu_speed 100000   //100khz通讯


int32_t tempAht = 300;
int32_t humAht = 300;
bool ahtStatusOK = false;
 
static const char* TAG = "aht1020";

int i2c_master_port=I2C_NUM_0;
 
void I2C_Init()
{
    i2c_config_t i2c_cnf={
    .mode=I2C_MODE_MASTER,
    .master.clk_speed=commu_speed,
    .scl_io_num=scl_gpio,
    .sda_io_num=sda_gpio,
    };
    i2c_param_config(i2c_master_port,&i2c_cnf);
    i2c_driver_install(i2c_master_port,I2C_MODE_MASTER,0,0,0);
}
 
/*AHT20刚上电时需要校准*/
void AHT20_Init()
{
    uint8_t status=0;
    uint8_t buffer[3]={0xBE,0x08,0x00};
    I2C_Init();
    vTaskDelay(100/portTICK_PERIOD_MS);
    i2c_cmd_handle_t cmd;
    cmd=i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, ((slave_addr << 1) | I2C_MASTER_WRITE), true);
    i2c_master_write(cmd,buffer,sizeof(buffer),true);
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(i2c_master_port, cmd, pdMS_TO_TICKS(50));
    i2c_cmd_link_delete(cmd);
 
    i2c_master_read_from_device(i2c_master_port,slave_addr,&status,1,pdMS_TO_TICKS(50));
    if ((status & 0x08) == 0x08){
        ESP_LOGI(TAG,"AHT20 is calibrated.");
        ahtStatusOK = true;
    }
    else{
        ESP_LOGI(TAG,"AHT20 calibrated failed.");
        ahtStatusOK = false;
    }
        
}


void TaskAHT10(void *arg)
{
    uint8_t busy_status=0xFF;
    uint8_t AC_CMD[3]={0xAC,0x33,0x00};
    uint8_t read_buf[6]={0};
    uint32_t rh_raw,temp_raw;
    float temp,rh;
    
    AHT20_Init();

    while(1)
    { 
        /*先发送AC命令触发测量*/
        i2c_master_write_to_device(i2c_master_port,slave_addr,AC_CMD,sizeof(AC_CMD),pdMS_TO_TICKS(50));
 
        /*等80ms测量完成*/
        vTaskDelay(80/portTICK_PERIOD_MS);
 
        uint8_t cnt=10;
        do/*检测AHT20是否处于忙状态*/
        {
            i2c_master_read_from_device(i2c_master_port,slave_addr,&busy_status,1,pdMS_TO_TICKS(5));
            vTaskDelay(2/portTICK_PERIOD_MS);
            cnt--;
        }while(((busy_status & 0x80) ==0x80 ) && (cnt>0));
        if(0==cnt)
        {
            ahtStatusOK = false;
            ESP_LOGE(TAG,"AHT20 is busy, read timeout.");
        }
        else{
            ahtStatusOK = true;

            /*读5个字节湿温度数据*/
            i2c_master_read_from_device(i2c_master_port,slave_addr,read_buf,sizeof(read_buf),pdMS_TO_TICKS(50));
    
            /*数据转换*/
            rh_raw=((uint32_t)read_buf[1]<<16) | ((uint32_t)read_buf[2]<<8) | ((uint32_t)read_buf[3]);
            rh_raw=rh_raw >> 4;
            temp_raw=((uint32_t)(read_buf[3]&0x0F)<<16) | ((uint32_t)read_buf[4]<<8) | ((uint32_t)read_buf[5]);
    
            /*数据计算*/
            rh=((uint64_t)rh_raw*10000)>>20;
            temp=(((uint64_t)temp_raw*20000)>>20)-5000;

            tempAht = temp / 10;
            humAht = temp / 10;

        }
        
        vTaskDelay(1000/portTICK_PERIOD_MS);

    }
}
 