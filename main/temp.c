#include "temp.h"

#include "tm1640.h"
#include "mqtt.h"
#include "beeper.h"
#include "led.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"

#include "global.h"

#include "aht10.h"

int currentTempC = 0;

int currentTempF = 0;


int convertCtoF10(int temp){
  return temp * 18 / 10 + 320;
}

int convertFtoC10(int temp){
  return (temp - 320) * 10 / 18;
}

void convertSettingsCtoF(){
  tempTarget = tempTarget * 18 / 10 + 320;
  tempDiff = tempDiff * 18 / 10;
  tempCal = tempCal * 18 / 10;
  tempAlarmH = tempAlarmH * 18 / 10 + 320;
  tempAlarmL = tempAlarmL * 18 / 10 + 320;
}

void convertSettingsFtoC(){
  tempTarget = (tempTarget - 320) * 10 / 18;
  tempDiff = tempDiff * 10 / 18;
  tempCal = tempCal * 10 / 18;
  tempAlarmH = (tempAlarmH - 320) * 10 / 18;
  tempAlarmL = (tempAlarmL - 320) * 10 / 18;
}





void updateCurrentToLCD(){
  if(tempUnit == 1){//华氏度
    TM1640_ShowTemp(currentTempF);
  }
  else{
    TM1640_ShowTemp(currentTempC);
  }
}

void updateTargetTempToLCD(){
  TM1640_ShowTarget(tempTarget);
}

void updateTempDiffToLCD(){
  TM1640_ShowDiff(tempDiff);
}

void updateTempCalToLCD(){
  TM1640_ShowCal(tempCal);
}

void updateTempAlarmHToLCD(){
  TM1640_ShowAlarmHigh(tempAlarmH);
}

void updateTempAlarmLToLCD(){
  TM1640_ShowAlarmLow(tempAlarmL);
}

//显示当前的Settings值
void showCurrentSettingLCD(){
  if(mode_setting == 0){
    //目标温度
    updateTargetTempToLCD();
  }
  else if(mode_setting == 1){
    //回差值
    updateTempDiffToLCD();
  }
  else if(mode_setting == 2){
    //修正值
    updateTempCalToLCD();
  }
  else if(mode_setting == 3){
    //高温警报值
    updateTempAlarmHToLCD();
  }
  else if(mode_setting == 4){
    //低温警报值
    updateTempAlarmLToLCD();
  }
}

void TaskTempCode( void * parameter)
{

  int32_t alarmStatusPrev = 0;
  int32_t temp_last = 0;
  int32_t temp_diff = 0;
  int32_t temp_now = 0;
  int32_t temp_upload = false;

  while(1){

    temp_upload = false;

	  vTaskDelay(1000 / portTICK_PERIOD_MS);   /* 延时2000ms*/

    if(!ahtStatusOK){

      TM1640_ShowTempError();
		  SET_ALARM_PROBE_ERROR();
      mqtt_upload_atonce = 1; //上报警报消息
      vTaskDelay(1000 / portTICK_PERIOD_MS);   /* 延时1000ms*/
      continue;

    }
    else{

      SET_ALARM_PROBE_OK();

      //上传一次
      temp_now = tempAht;

      temp_diff = temp_now - temp_last;
      temp_diff = abs(temp_diff);

      if(temp_now - temp_last > 1){
        temp_upload = true;
      }

      //摄氏度
      currentTempC = tempAht;
      //华氏度
      currentTempF = convertCtoF10(currentTempC);

      //****以上是未加修正值的温度*****//

      //****以下是加上修正值后****//

      if(tempUnit == 1){//华氏度
        //加上修正值
        currentTempF = currentTempF + tempCal;//华氏度不丢失精度
        currentTempC = convertFtoC10(currentTempF);//摄氏度允许丢失精度
      }
      else{//摄氏度
        //加上修正值
        currentTempC = currentTempC + tempCal;//摄氏度不丢失精度
        currentTempF = convertCtoF10(currentTempC);//华氏度允许丢失精度
      }

      printf("currentTempC: %d\n", currentTempC);
      printf("currentTempF: %d\n", currentTempF);

      //更新到显示屏
      updateCurrentToLCD();
      
      showCurrentSettingLCD();


      alarmStatusPrev = alarmStatus;
      //温度警报
      if(tempUnit == 1){//华氏度
        if(currentTempF > tempAlarmH){
          SET_ALARM_HIGH_ON();
          SET_ALARM_LOW_OFF();
          //mode_setting = 3;
        }
        else if(currentTempF < tempAlarmL){
          SET_ALARM_LOW_ON();
          SET_ALARM_HIGH_OFF();
          //mode_setting = 4;
        }
        else{
          SET_ALARM_HIGH_OFF();
          SET_ALARM_LOW_OFF();
          alarmSuspend = 0;//回到正常温度范围时，取消可能的人为静默
        }
      }
      else{//摄氏度
        if(currentTempC > tempAlarmH){
          SET_ALARM_HIGH_ON();
          SET_ALARM_LOW_OFF();
          //mode_setting = 3;
        }
        else if(currentTempC < tempAlarmL){
          SET_ALARM_LOW_ON();
          SET_ALARM_HIGH_OFF();
          //mode_setting = 4;
        }
        else{
          SET_ALARM_HIGH_OFF();
          SET_ALARM_LOW_OFF();
          alarmSuspend = 0;//回到正常温度范围时，取消可能的人为静默
        }
      }

      if(alarmStatusPrev != alarmStatus || temp_upload){
        mqtt_upload_atonce = 1;
      }

    }

  }

}
