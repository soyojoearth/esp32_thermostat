#include "btn.h"
#include "mqtt.h"
#include "led.h"
#include "tm1640.h"
#include "temp.h"
#include "beeper.h"

#include "nvs_flash.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"

#include "global.h"

void saveWifiMode(){

	// 打开 NVS 存储空间
	nvs_handle nvs_handler;
	nvs_open("cfg_storage", NVS_READWRITE, &nvs_handler);

	nvs_set_i32(nvs_handler, "wifiMode", wifiMode);

	nvs_commit(nvs_handler);

	nvs_close(nvs_handler);

}

void saveSettings(){

	// 打开 NVS 存储空间
	nvs_handle nvs_handler;
	nvs_open("cfg_storage", NVS_READWRITE, &nvs_handler);
	nvs_set_i32(nvs_handler, "tempUnit", tempUnit);
	nvs_set_i32(nvs_handler, "tempTarget", tempTarget);
	nvs_set_i32(nvs_handler, "tempDiff", tempDiff);
	nvs_set_i32(nvs_handler, "tempCal", tempCal);
	nvs_set_i32(nvs_handler, "tempAlarmH", tempAlarmH);
	nvs_set_i32(nvs_handler, "tempAlarmL", tempAlarmL);

	nvs_commit(nvs_handler);

	nvs_close(nvs_handler);

}

int pressTimes = 0;//记录按键长按时间

//按键控制
void TaskBtnCode( void * parameter)
{
	while(gpio_get_level(btn_temp) == 0)//按键松开后，再运行后面的
	{
		vTaskDelay(20 / portTICK_PERIOD_MS);
	}

    while(1){
      vTaskDelay(10 / portTICK_PERIOD_MS);//去抖
       //按键控制灯
      if(gpio_get_level(btn) == 0){
    	  vTaskDelay(20 / portTICK_PERIOD_MS);//去抖
          if(gpio_get_level(btn) == 0){
            while(gpio_get_level(btn) == 0);
            //短按
            if(mode_setting < 4){
              mode_setting++;
            }
            else{
              mode_setting = 0;
            }
            showCurrentSettingLCD();
            alarmSuspend = 1;//按任意键，警报临时静默
          }
      }

      if(gpio_get_level(btn_decrease) == 0){
          pressTimes = 0;
          vTaskDelay(20 / portTICK_PERIOD_MS);//去抖
          if(gpio_get_level(btn_decrease) == 0){
            while(gpio_get_level(btn_decrease) == 0)
            {
            	vTaskDelay(20 / portTICK_PERIOD_MS);//去抖
                pressTimes += 20;
                if(pressTimes > 1000){//长按

                  while(gpio_get_level(btn_decrease) == 0)
                  {
                    //连续减小
                    if(mode_setting == 0){
                      tempTarget--;//目标值
                    }
                    else if(mode_setting == 1){
                      tempDiff--;//回差值
                      if(tempDiff < 0){ tempDiff = 0; SET_BEEP(3);}
                      if(tempDiff > 99){ tempDiff = 99; SET_BEEP(3);}
                    }
                    else if(mode_setting == 2){
                      tempCal--;//修正值
                      if(tempCal < -99){ tempCal = -99; SET_BEEP(3);}
                      if(tempCal > 99){ tempCal = 99; SET_BEEP(3);}
                    }
                    else if(mode_setting == 3){
                      tempAlarmH-=10;//高温警报值
                      if(tempAlarmH > 3000){ tempAlarmH = 3000; SET_BEEP(3);}
                      if(tempAlarmH < tempAlarmL){ tempAlarmH = tempAlarmL; SET_BEEP(3);}
                    }
                    else if(mode_setting == 4){
                      tempAlarmL-=10;//低温警报值
                      if(tempAlarmL < -400){ tempAlarmL = -400; SET_BEEP(3);}
                      if(tempAlarmL > tempAlarmH){ tempAlarmL = tempAlarmH; SET_BEEP(3);}
                    }
                    vTaskDelay(50 / portTICK_PERIOD_MS);
                    showCurrentSettingLCD();
                  }
                }
            }
            if(pressTimes < 1000){
              //短按
              if(mode_setting == 0){
                tempTarget--;//目标值
              }
              else if(mode_setting == 1){
                tempDiff--;//回差值
                if(tempDiff < 0){ tempDiff = 0; SET_BEEP(3);}
                if(tempDiff > 99){ tempDiff = 99; SET_BEEP(3);}
              }
              else if(mode_setting == 2){
                tempCal--;//修正值
                if(tempCal < -99){ tempCal = -99; SET_BEEP(3);}
                if(tempCal > 99){ tempCal = 99; SET_BEEP(3);}
              }
              else if(mode_setting == 3){
                tempAlarmH-=10;//高温警报值
                if(tempAlarmH > 3000){ tempAlarmH = 3000; SET_BEEP(3);}
                if(tempAlarmH < tempAlarmL){ tempAlarmH = tempAlarmL; SET_BEEP(3);}
              }
              else if(mode_setting == 4){
                tempAlarmL-=10;//低温警报值
                if(tempAlarmL < -400){ tempAlarmL = -400; SET_BEEP(3);}
                if(tempAlarmL > tempAlarmH){ tempAlarmL = tempAlarmH; SET_BEEP(3);}
              }
              showCurrentSettingLCD();
            }
            //保存
            saveSettings();
            mqtt_upload_atonce = 1;
            alarmSuspend = 1;//按任意键，警报临时静默
          }
      }
//
      if(gpio_get_level(btn_increase) == 0){
          pressTimes = 0;
          vTaskDelay(20 / portTICK_PERIOD_MS);//去抖
          if(gpio_get_level(btn_increase) == 0){
            while(gpio_get_level(btn_increase) == 0)
            {
            	vTaskDelay(20 / portTICK_PERIOD_MS);
                pressTimes += 20;
                if(pressTimes > 1000){//长按
                  printf("btn_increase Long Press\n");
                  while(gpio_get_level(btn_increase) == 0)
                  {
                    //连续增加
                    if(mode_setting == 0){
                      tempTarget++;//目标值
                    }
                    else if(mode_setting == 1){
                      tempDiff++;//回差值
                      if(tempDiff < 0){ tempDiff = 0; SET_BEEP(3);}
                      if(tempDiff > 99){ tempDiff = 99; SET_BEEP(3);}
                    }
                    else if(mode_setting == 2){
                      tempCal++;//修正值
                      if(tempCal < -99){ tempCal = -99; SET_BEEP(3);}
                      if(tempCal > 99){ tempCal = 99; SET_BEEP(3);}
                    }
                    else if(mode_setting == 3){
                      tempAlarmH+=10;//高温警报值
                      if(tempAlarmH > 3000){ tempAlarmH = 3000; SET_BEEP(3);}
                      if(tempAlarmH < tempAlarmL){ tempAlarmH = tempAlarmL; SET_BEEP(3);}
                    }
                    else if(mode_setting == 4){
                      tempAlarmL+=10;//低温警报值
                      if(tempAlarmL < -400){ tempAlarmL = -400; SET_BEEP(3);}
                      if(tempAlarmL > tempAlarmH){ tempAlarmL = tempAlarmH; SET_BEEP(3);}
                    }
                    vTaskDelay(50 / portTICK_PERIOD_MS);
                    showCurrentSettingLCD();
                  }
                }
            }
            if(pressTimes < 1000){
              //短按
              if(mode_setting == 0){
                tempTarget++;//目标值
              }
              else if(mode_setting == 1){
                tempDiff++;//回差值
                if(tempDiff < 0){ tempDiff = 0; SET_BEEP(3);}
                if(tempDiff > 99){ tempDiff = 99; SET_BEEP(3);}
              }
              else if(mode_setting == 2){
                tempCal++;//修正值
                if(tempCal < -99){ tempCal = -99; SET_BEEP(3);}
                if(tempCal > 99){ tempCal = 99; SET_BEEP(3);}
              }
              else if(mode_setting == 3){
                tempAlarmH+=10;//高温警报值
                if(tempAlarmH > 3000){ tempAlarmH = 3000; SET_BEEP(3);}
                if(tempAlarmH < tempAlarmL){ tempAlarmH = tempAlarmL; SET_BEEP(3);}
              }
              else if(mode_setting == 4){
                tempAlarmL+=10;//低温警报值
                if(tempAlarmL < -400){ tempAlarmL = -400; SET_BEEP(3);}
                if(tempAlarmL > tempAlarmH){ tempAlarmL = tempAlarmH; SET_BEEP(3);}
              }
              showCurrentSettingLCD();
            }
            //保存
            saveSettings();
            mqtt_upload_atonce = 1;
            alarmSuspend = 1;//按任意键，警报临时静默
          }
      }

      if(gpio_get_level(btn_temp) == 0){
          pressTimes = 0;
          vTaskDelay(20 / portTICK_PERIOD_MS);//去抖
          if(gpio_get_level(btn_temp) == 0){
            while(gpio_get_level(btn_temp) == 0)
            {
            	vTaskDelay(20 / portTICK_PERIOD_MS);
                pressTimes += 20;
                if(pressTimes > 2000){//长按
                  printf("btn Long Press");
                  pressTimes = 0;
                  unpairDevice();//unpair wifi & ble
                  if(wifiMode == 0){
                      wifiMode = 2;//切换到蓝牙
                  }
                  // else if(wifiMode == 2){
                  //     wifiMode = 1;//切换到WiFi热点
                  // }
                  else{
                      wifiMode = 0;//切换回普通WiFi模式
                  }
                  saveWifiMode();
                  vTaskDelay(2000 / portTICK_PERIOD_MS);   /* 延时2000ms*/
                  printf("Reset");
                  esp_restart();//复位

                }
            }

            if(pressTimes < 5000){
              //短按
              if(tempUnit == 0){
                tempUnit = 1;
                convertSettingsCtoF();
                saveSettings();
              }
              else{
                tempUnit = 0;
                convertSettingsFtoC();
                saveSettings();
              }

              updateCurrentToLCD();
              showCurrentSettingLCD();

              if(tempUnit == 1){//华氏度，蓝灯
                led_F();
              }
              else{//摄氏度，黄灯
                led_C();
              }

              mqtt_upload_atonce = 1;

            }

            alarmSuspend = 1;//按任意键，警报临时静默

          }
      }

    }

}
