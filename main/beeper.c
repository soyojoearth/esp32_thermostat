#include "beeper.h"
#include "mqtt.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"

#include "global.h"


int beepTime = 0;

int alarmCounter = 0;

int alarmStatus = 0;//警报状态值，从低位到高位: 0高温|1低温|2探针异常

int alarmSuspend = 0;//临时人为静默

void SET_ALARM_PROBE_ERROR(){
  if(!(alarmStatus & (1<<2))){
	//第一次触发
	alarmStatus |= (1<<2);
  }
}

void SET_ALARM_PROBE_OK(){
  if((alarmStatus & (1<<2))){
    //第一次关闭
    alarmStatus &= ~(1<<2);
  }
}

void SET_ALARM_HIGH_ON(){
  if(!(alarmStatus & 0x01)){
    //第一次触发
    alarmStatus |= 0x01;
  }
}

void SET_ALARM_HIGH_OFF(){
  if((alarmStatus & 0x01)){
    //第一次关闭
    alarmStatus &= ~0x01;
  }
}

void SET_ALARM_LOW_ON(){
  if(!(alarmStatus & 0x02)){
    //第一次触发
    alarmStatus |= 0x02;
  }
}

void SET_ALARM_LOW_OFF(){
  if((alarmStatus & 0x02)){
    //第一次关闭
    alarmStatus &= ~0x02;
  }
}

void SET_BEEP(int t){
  if(t > beepTime){
    beepTime = t;
  }
}

void BEEP_ON() {
	gpio_set_level(beeper_io, 1);
}

void BEEP_OFF(){
	gpio_set_level(beeper_io, 0);
}

void TaskBeeperCode( void * parameter)
{
  while(1){
    if(beepTime>0){
      BEEP_ON();
      beepTime--;
    }
    else{
      BEEP_OFF();
    }
    if(alarmStatus){//警报声，1秒一次
      if(alarmCounter < 5){
        BEEP_ON();
      }
      else{
        BEEP_OFF();
      }
      alarmCounter++;//计数器
      if(alarmCounter >= 10){
        alarmCounter = 0;
      }
    }
    if(alarmSuspend){
      BEEP_OFF();//临时人为静默
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}
