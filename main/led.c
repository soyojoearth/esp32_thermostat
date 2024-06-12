#include "led.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"

#include "global.h"

int t_led_on = 0;
int t_led_off = 0;

int led_mode = 0;

void led_C(){
	// turn on led C
}

void led_F(){
	// turn on led F
}

void led_on(){
	gpio_set_level(led, 1);
}
void led_off(){
	gpio_set_level(led, 0);
}

//wifi connecting
void setLedStatusConnecting(){
  led_mode = 0x02;
}

//wifi connected
void setLedStatusConnected(){
  led_mode = 0x03;
}

//Mode BLE Pairing
void setLedStatusBluetooth(){
  led_mode = 0x00;
}

//WiFi Hot spot
void setLedStatusWiFiHot(){
  led_mode = 0x01;
}

void setLedOnOffTime(int t_on,int t_off){
  t_led_on = t_on;
  t_led_off = t_off;
}

void TaskLedStatusCode( void * parameter)
{
  static int t_led_count = 0;
  while(1){
    
      if(led_mode == 0x00){
         t_led_on = 25;
         t_led_off = 25;
      }
      else if(led_mode == 0x01){
         t_led_on = 150;
         t_led_off = 150;
      }
      else if(led_mode == 0x02){
         t_led_on = 0;
         t_led_off = 1000;
      }
      else if(led_mode == 0x03){
         t_led_on = 1000;
         t_led_off = 0;
      }
      else if(led_mode == 0x04){
         t_led_on = 1000;
         t_led_off = 0;
      }
      else if(led_mode == 0x05){
          t_led_on = 0;
         t_led_off = 1000;
      }
      else if(led_mode == 0x06){
         t_led_on = 25;
         t_led_off = 25;
      }

      //----------------------------//

      if(t_led_count <= t_led_on){
        led_on();
      }
      else if(t_led_count > t_led_on  && t_led_count < (t_led_on+t_led_off)){
        led_off();
      }
      else if(t_led_count >= (t_led_on+t_led_off)){
        t_led_count = 0;
      }
      
      vTaskDelay(10 / portTICK_PERIOD_MS);
      t_led_count++;

      //-----------------------------//

      if(tempUnit == 1){
        led_F();
      }
      else{
        led_C();
      }

  }
}


