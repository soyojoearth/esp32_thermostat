#include "tm1640.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"

#include "driver/gpio.h"


/**
  ***************************************************************************************
  * TM1640模块初始化头文件
  * TubePoint：数码管显示数字是否带小数点
  * LightState：LED灯亮灭状态
  * TM1640_Init()：TM1640初始化函数
  * TM1640_WriteData()：TM1640写数据函数
  * TM1640_WriteAddressData()：TM1640指定地址写数据函数
  * TM1640_Tube()：TM1640指定数码管序号与显示数字函数
  * TM1640_TubeOff()：TM1640关闭指定数码管函数
  ***************************************************************************************
  */
#define LIGHT_WEIGHT 0x8F//亮度等级，最高

// DIN - TM1640_DATA

#define DIN_HIGH() gpio_set_level(TM1640_DATA, 1)//置1

#define DIN_LOW() gpio_set_level(TM1640_DATA, 0)//置0

// CLK - TM1640_CLK

#define CLK_HIGH() gpio_set_level(TM1640_CLK, 1)//置1

#define CLK_LOW() gpio_set_level(TM1640_CLK, 0)//置0

typedef  unsigned char uint8;

uint8 tm_utime = 5;//bit位间隔时间

uint8 point = 1;//是否要输出小数点（小数点闪动标记）

typedef enum {NoPoint = 0, Point = !NoPoint} TubePoint; //是否带小数点，Point：带，NoPoint：不带

void TM1640_WriteData(unsigned char data); //TM1640写数据函数

void TM1640_WriteAddressData(unsigned char addr,unsigned char data); //TM1640指定地址写数据函数

void TM1640_Tube(unsigned char serial, unsigned char num, TubePoint point); //TM1640指定数码管序号与显示数字函数

void TM1640_TubeOff(unsigned char serial);  //TM1640关闭指定数码管函数

void TM1640_ShowNumPreCode(char code, signed short value, TubePoint point);//显示带前缀符号的3位数字


void TM1640_Init(void) //TM1640初始化函数
{

	gpio_config_t gpio_config_structure;

	gpio_config_structure.pin_bit_mask = (1ULL << TM1640_CLK);/* 选择gpio */
	gpio_config_structure.mode = GPIO_MODE_OUTPUT;              /* 输出模式 */
	gpio_config_structure.pull_up_en = 0;                       /* 不上拉 */
	gpio_config_structure.pull_down_en = 0;                     /* 不下拉 */
	gpio_config_structure.intr_type = GPIO_PIN_INTR_DISABLE;    /* 禁止中断 */
	gpio_config(&gpio_config_structure);

	gpio_config_structure.pin_bit_mask = (1ULL << TM1640_DATA);/* 选择gpio */
	gpio_config_structure.mode = GPIO_MODE_OUTPUT;              /* 输出模式 */
	gpio_config_structure.pull_up_en = 0;                       /* 不上拉 */
	gpio_config_structure.pull_down_en = 0;                     /* 不下拉 */
	gpio_config_structure.intr_type = GPIO_PIN_INTR_DISABLE;    /* 禁止中断 */
	gpio_config(&gpio_config_structure);

}

void TM1640_Start(void)
{
  //数据输入的开始条件是CLK 为高电平时，DIN 由高变低；
  CLK_LOW();
  DIN_HIGH();
  CLK_HIGH();
  ets_delay_us(tm_utime);
  DIN_LOW();
  ets_delay_us(tm_utime);

}

void TM1640_End(void)
{
  CLK_LOW();
  //结束条件是CLK 为高时，DIN 由低电平变为高
  DIN_LOW();
  CLK_HIGH();
  ets_delay_us(tm_utime);
  DIN_HIGH();
  ets_delay_us(tm_utime);
}

/***************************************************************************************/
/**
  * TM1640写数据函数
  * 参数：data：要写入的8位数据
  * 返回值：无
  */
void TM1640_WriteData(unsigned char data) //TM1640写数据函数
{

  //CLK的上升延输入数据
  unsigned char i;
  ets_delay_us(tm_utime);
  for(i=0;i<8;i++)
  {
      CLK_LOW(); //CLK=0
      ets_delay_us(tm_utime);
      if(data&0x01)
      {
          DIN_HIGH(); //DIO=1
      }
      else
      {
          DIN_LOW(); //DIO=0
      }
      data>>=1;
      CLK_HIGH(); //CLK=1
      ets_delay_us(tm_utime);
  }

}

/**
  * TM1640指定地址写数据函数
  * 参数1：addr：要写入数据的地址
  * 参数2：data：要写入的8位数据
  * 返回值：无
  */
void TM1640_WriteAddressData(unsigned char addr,unsigned char data) //TM1640指定地址写数据函数
{
  TM1640_WriteData(addr); //地址
  TM1640_WriteData(data); //数据
}


unsigned char TM1640_LED[]={0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,  //共阴极数码管段码，不带小数点
                            0x7F,0x6F,0x77,0x7C,0x39,0x5E,0x79,0x71}; //0～F，1亮0灭

unsigned char TM1640_LED_P[]={0xBF,0x86,0xDB,0xCF,0xE6,0xED,0xFD,0x87,  //共阴极数码管段码，带小数点
                            0xFF,0xEF,0xF7,0xFC,0xB9,0xDE,0xF9,0xF1}; //0～F，1亮0灭


/**
  * TM1640指定数码管序号与显示数字函数
  * 参数1：serial：数码管序号，1-8
  * 参数2：num：要显示的数字，0-F
  * 参数3：point：是否带小数点，Point：带，NoPoint：不带
  * 返回值：无
  */
void TM1640_Tube(unsigned char serial, unsigned char num, TubePoint point) //TM1640指定数码管序号与显示数字函数
{

  TM1640_Start();

  TM1640_WriteData(0x44); //固定地址，写数据到显示寄存器

  TM1640_End();

  TM1640_Start();

  if(point == Point)  //带小数点
  {
      TM1640_WriteAddressData(0XC0+(serial-1),TM1640_LED_P[num]); //第serial个数码管显示num，带小数点
  }
  else if(point == NoPoint)  //不带小数点
  {
      TM1640_WriteAddressData(0XC0+(serial-1),TM1640_LED[num]); //第serial个数码管显示num，不带小数点
  }

  TM1640_End();

  TM1640_Start();
  TM1640_WriteData(LIGHT_WEIGHT); //显示开，亮度第1级
  TM1640_End();
}


void TM1640_Tube_Code(unsigned char serial, unsigned char code) //TM1640指定数码管序号与显示内容
{

  TM1640_Start();

  TM1640_WriteData(0x44); //固定地址，写数据到显示寄存器

  TM1640_End();

  TM1640_Start();

  TM1640_WriteAddressData(0XC0+(serial-1),code); //第serial个数码管显示num，带小数点

  TM1640_End();

  TM1640_Start();
  TM1640_WriteData(LIGHT_WEIGHT); //显示开，亮度第1级
  TM1640_End();
}

/**
  * TM1640关闭指定数码管函数
  * 参数：serial：数码管序号
  * 返回值：无
  */
void TM1640_TubeOff(unsigned char serial)  //TM1640关闭指定数码管函数
{

  TM1640_Start();
  TM1640_WriteData(0x44); //固定地址，写数据到显示寄存器
  TM1640_End();

  TM1640_Start();
  TM1640_WriteAddressData(0XC0+(serial-1),0x00); //第serial个数码管灭
  TM1640_End();

  TM1640_Start();
  TM1640_WriteData(LIGHT_WEIGHT); //显示开，亮度第1级
  TM1640_End();


}

//显示负号
void TM1640_Tube_Negative(unsigned char serial)
{
  TM1640_Start();
  TM1640_WriteData(0x44); //普通模式，固定地址，写数据到显示寄存器
  TM1640_End();

  TM1640_Start();
  TM1640_WriteAddressData(0XC0+(serial-1),0x40);
  TM1640_End();

  TM1640_Start();
  TM1640_WriteData(LIGHT_WEIGHT); //显示开，亮度第1级
  TM1640_End();
}

void TM1640_ShowTempError()//读取温度错误
{
    TM1640_Tube_Code(1, 0x79);//显示字母E
    TM1640_Tube_Code(2, 0x77);//显示字母R
    TM1640_Tube_Code(3, 0x3f);//显示字母O
    TM1640_Tube_Code(4, 0x77);//显示字母R
}

void TM1640_ShowTemp(signed short value)//显示温度，温度放大10倍
{
	TM1640_TubeOff(1);
	TM1640_TubeOff(2);
	TM1640_TubeOff(3);
	TM1640_TubeOff(4);

    if(value < -400){
          value = -400;
    }
    if(value > 9999){
          value = 9999;
    }

    if(value < 0){
          //显示负号；负数没有百位，因为不会低于-40度
          TM1640_Tube_Negative(1);
          value = value * -1;
    }

	if(value >= 1000){
	  TM1640_Tube(1,value/1000%10,NoPoint);
	}
	if(value >= 100){
	  TM1640_Tube(2,value/100%10,NoPoint);
	}

    if(point){
        TM1640_Tube(3,value/10%10,Point);
        point = !point;
    }
    else{
        TM1640_Tube(3,value/10%10,NoPoint);
        point = !point;
    }
    TM1640_Tube(4,value%10,NoPoint);

}

void TM1640_ShowTarget(signed short value)//显示目标温度
{

	TM1640_TubeOff(5);
	TM1640_TubeOff(6);
	TM1640_TubeOff(7);
	TM1640_TubeOff(8);

    if(value < -400){
          value = -400;
    }
    if(value > 9999){
          value = 9999;
    }

    //是否显示负号
    if(value < 0){
          //显示负号；负数没有百位，因为不会低于-40度
          TM1640_Tube_Negative(5);
          value = value * -1;
    }

	if(value >= 1000){
	  TM1640_Tube(5,value/1000%10,NoPoint);
	}
	if(value >= 100){
	  TM1640_Tube(6,value/100%10,NoPoint);
	}

    TM1640_Tube(7,value/10%10,Point);
    TM1640_Tube(8,value%10,NoPoint);

}

void TM1640_ShowDiff(signed short value)//显示回差值.
{

    if(value < 0){
          value = 0;
    }

    if(value > 99){
          value = 99;
    }

    TM1640_Start();
    TM1640_WriteData(0x44); //普通模式，固定地址，写数据到显示寄存器
    TM1640_End();

    TM1640_Start();
    TM1640_WriteData(LIGHT_WEIGHT); //显示开，亮度第1级
    TM1640_End();

    TM1640_Tube_Code(5, 0x5E);//显示字母D

    if(value/100%10 > 0){
          TM1640_Tube(6,value/100%10,NoPoint);
    }
    else{
          TM1640_TubeOff(6);
    }

    TM1640_Tube(7,value/10%10,Point);

    TM1640_Tube(8,value%10,NoPoint);

}

void TM1640_ShowCal(signed short value)//显示修正值
{
    if(value < -99){
          value = -99;
    }

    if(value > 99){
          value = 99;
    }

    TM1640_Start();
    TM1640_WriteData(0x44); //普通模式，固定地址，写数据到显示寄存器
    TM1640_End();

    TM1640_Start();
    TM1640_WriteData(LIGHT_WEIGHT); //显示开，亮度第1级
    TM1640_End();


    TM1640_Tube_Code(5, 0x39);//显示字母C

    if(value < 0){
          //显示负号；负数没有百位，因为不会低于-40度

          TM1640_Tube_Negative(6);

          value = value * -1;
    }
    else {
          TM1640_TubeOff(6);
    }

    TM1640_Tube(7,value/10%10,Point);
    TM1640_Tube(8,value%10,NoPoint);
}

void TM1640_ShowAlarmHigh(signed short value)//显示高温警报值
{
  value = value / 10;
  TM1640_ShowNumPreCode(0x76,value,NoPoint);
}

void TM1640_ShowAlarmLow(signed short value)//显示低温警报值
{
  value = value / 10;
  TM1640_ShowNumPreCode(0x38,value,NoPoint);
}


void TM1640_ShowNumPreCode(char code, signed short value, TubePoint point)//显示带前置符号的数字，不超过3位
{

  TM1640_Start();
  TM1640_WriteData(0x44); //普通模式，固定地址，写数据到显示寄存器
  TM1640_End();

  TM1640_Start();
  TM1640_WriteData(LIGHT_WEIGHT); //显示开，亮度第1级
  TM1640_End();

  TM1640_Tube_Code(5, code);//显示前置符号

  if(value >= 0){//正数类型
    if(value >= 100){
      TM1640_Tube(6,value/100%10,NoPoint);
    }
    else{
      TM1640_TubeOff(6);
    }
    if(value >= 10){
      TM1640_Tube(7,value/10%10,point);
    }
    else{
      TM1640_TubeOff(7);
    }
    TM1640_Tube(8,value%10,NoPoint);
  }
  else{//负数类型

    value = value * -1;//去掉负号

    short n_index = 6;//计算负号位置
    if(value >= 10){
      n_index = 6;
    }
    else{
      n_index = 7;
      TM1640_TubeOff(6);
    }

    //显示负号
    TM1640_Tube_Negative(n_index);

    if(value >= 10){
      TM1640_Tube(7,value/10%10,point);
    }

    TM1640_Tube(8,value%10,NoPoint);

  }
}

