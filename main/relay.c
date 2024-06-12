#include "relay.h"
#include "temp.h"
#include "mqtt.h"
#include "beeper.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"

#include "global.h"


int isHeating = 0;
int isCooling = 0;

int delay_seconds = 0;

void setRelayOff(){
	gpio_set_level(relay_heat_io, 0);
	gpio_set_level(relay_cool_io, 0);
}

void setRelayCool(){
	gpio_set_level(relay_heat_io, 0);
	gpio_set_level(relay_cool_io, 1);
}

void setRelayHeat(){
	gpio_set_level(relay_cool_io, 0);
	gpio_set_level(relay_heat_io, 1);
}

void TaskRelayCode( void * parameter)
{
  static int currentTempRelay;

  vTaskDelay(1000 / portTICK_PERIOD_MS);   /* 延时1000ms*/
  //等取到温度后再启动

  while(1){

	if(alarmStatus & (1<<2)){
		//探针异常
		setRelayOff();
		vTaskDelay(100 / portTICK_PERIOD_MS);   /* 延时100ms*/
		continue;
	}

    if(tempUnit == 1){
      currentTempRelay = currentTempF;
    }
    else{
      currentTempRelay = currentTempC;
    }

    if(isCooling){//判断何时停止输出
      if(currentTempRelay > tempTarget){
        //制冷保持开启
        setRelayCool();
      }
      else{
        if(delay_seconds > 0){
          delay_seconds--;
        }
        else{
          //恢复正常
          setRelayOff();
          isCooling = 0;
          mqtt_upload_atonce = 1;
        }
      }
    }
    else if(isHeating){//判断何时停止输出
      if(currentTempRelay < tempTarget){
        //制热保持开启
        setRelayHeat();
      }
      else{
        if(delay_seconds > 0){
          delay_seconds--;
        }
        else{
            //恢复正常
          setRelayOff();
          isHeating = 0;
          mqtt_upload_atonce = 1;
        }
      }
    }
    else{//监测是否开启输出
      if(currentTempRelay > (tempTarget+tempDiff)){
        //输出开启，制冷模式
        isCooling = 1;
        isHeating = 0;
        mqtt_upload_atonce = 1;
        //cooling Delay
        if (coolingDelay > 0)
        {
          delay_seconds = coolingDelay * 60;
        }
        
      }
      if(currentTempRelay < (tempTarget-tempDiff)){
        //输出开启，制热模式
        isHeating = 1;
        isCooling = 0;
        mqtt_upload_atonce = 1;
        //heatingDelay
        if (heatingDelay > 0)
        {
          delay_seconds = heatingDelay * 60;
        }
      }
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS);   /* 延时1000ms*/

  }
}
