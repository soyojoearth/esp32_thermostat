#include "global.h"

#include "driver/gpio.h"

#include "esp_adc_cal.h"

int runningTime = 0;
int countWifiConnected = 0;
int countMqttConnected = 0;

int wifiMode = 0;//0普通模式，1配网模式
int power = 1;//开机默认灯亮

int mode_setting = 0;//0目标温度，1回差值，2修正值

//以下值，全部放大10倍
int tempTarget=180;//目标值
int tempDiff=15;//回差值
int tempCal=0;//修正值
int tempAlarmH=3000;//高温警报值
int tempAlarmL=-400;//低温警报值

int coolingDelay=0;//cooling Delay
int heatingDelay=0;//heating Delay

int tempUnit = 0;//0摄氏度 1华氏度


void initIO(){

    gpio_config_t gpio_config_structure;

	//GPIO 0 默认上拉
    gpio_config_structure.pin_bit_mask = (1ULL << 0);/* 选择gpio0 */
    gpio_config_structure.mode = GPIO_MODE_INPUT;               /* 输入模式 */
    gpio_config_structure.pull_up_en = 1;                       /* 上拉 */
    gpio_config_structure.pull_down_en = 0;                     /* 不下拉 */
    gpio_config_structure.intr_type = GPIO_PIN_INTR_DISABLE;    /* 禁止中断 */

	gpio_config(&gpio_config_structure);

	//系统灯
    gpio_config_structure.pin_bit_mask = (1ULL << led);/* 选择gpio */
    gpio_config_structure.mode = GPIO_MODE_OUTPUT;              /* 输出模式 */
    gpio_config_structure.pull_up_en = 0;                       /* 不上拉 */
    gpio_config_structure.pull_down_en = 0;                     /* 不下拉 */
    gpio_config_structure.intr_type = GPIO_PIN_INTR_DISABLE;    /* 禁止中断 */
	gpio_config(&gpio_config_structure);

	//蜂鸣器
    gpio_config_structure.pin_bit_mask = (1ULL << beeper_io);/* 选择gpio */
    gpio_config_structure.mode = GPIO_MODE_OUTPUT;              /* 输出模式 */
    gpio_config_structure.pull_up_en = 0;                       /* 不上拉 */
    gpio_config_structure.pull_down_en = 0;                     /* 不下拉 */
    gpio_config_structure.intr_type = GPIO_PIN_INTR_DISABLE;    /* 禁止中断 */
	gpio_config(&gpio_config_structure);

	gpio_set_level(beeper_io, 0);//默认低电平


	//继电器 relay_heat_io
    gpio_config_structure.pin_bit_mask = (1ULL << relay_heat_io);/* 选择gpio */
    gpio_config_structure.mode = GPIO_MODE_OUTPUT;              /* 输出模式 */
    gpio_config_structure.pull_up_en = 0;                       /* 不上拉 */
    gpio_config_structure.pull_down_en = 0;                     /* 不下拉 */
    gpio_config_structure.intr_type = GPIO_PIN_INTR_DISABLE;    /* 禁止中断 */
	gpio_config(&gpio_config_structure);

	gpio_set_level(relay_heat_io, 0);//默认关闭

	//继电器 relay_cool_io
    gpio_config_structure.pin_bit_mask = (1ULL << relay_cool_io);/* 选择gpio */
    gpio_config_structure.mode = GPIO_MODE_OUTPUT;              /* 输出模式 */
    gpio_config_structure.pull_up_en = 0;                       /* 不上拉 */
    gpio_config_structure.pull_down_en = 0;                     /* 不下拉 */
    gpio_config_structure.intr_type = GPIO_PIN_INTR_DISABLE;    /* 禁止中断 */
	gpio_config(&gpio_config_structure);

	gpio_set_level(relay_cool_io, 0);//默认关闭


	//系统按键
    gpio_config_structure.pin_bit_mask = (1ULL << btn);/* 选择gpio0 */
    gpio_config_structure.mode = GPIO_MODE_INPUT;               /* 输入模式 */
    gpio_config_structure.pull_up_en = 1;                       /* 上拉 */
    gpio_config_structure.pull_down_en = 0;                     /* 不下拉 */
    gpio_config_structure.intr_type = GPIO_PIN_INTR_DISABLE;    /* 禁止中断 */

	gpio_config(&gpio_config_structure);


	//温标切换
    gpio_config_structure.pin_bit_mask = (1ULL << btn_temp);/* 选择gpio0 */
    gpio_config_structure.mode = GPIO_MODE_INPUT;               /* 输入模式 */
    gpio_config_structure.pull_up_en = 1;                       /* 上拉 */
    gpio_config_structure.pull_down_en = 0;                     /* 不下拉 */
    gpio_config_structure.intr_type = GPIO_PIN_INTR_DISABLE;    /* 禁止中断 */

	gpio_config(&gpio_config_structure);

	//增加
    gpio_config_t gpio_config_structure_btn_increase;
    gpio_config_structure_btn_increase.pin_bit_mask = (1ULL << btn_increase);/* 选择gpio0 */
    gpio_config_structure_btn_increase.mode = GPIO_MODE_INPUT;               /* 输入模式 */
    gpio_config_structure_btn_increase.pull_up_en = 1;                       /* 上拉 */
    gpio_config_structure_btn_increase.pull_down_en = 0;                     /* 不下拉 */
    gpio_config_structure_btn_increase.intr_type = GPIO_PIN_INTR_DISABLE;    /* 禁止中断 */

	gpio_config(&gpio_config_structure_btn_increase);

	//减小
    gpio_config_t gpio_config_structure_btn_decrease;
    gpio_config_structure_btn_decrease.pin_bit_mask = (1ULL << btn_decrease);/* 选择gpio0 */
    gpio_config_structure_btn_decrease.mode = GPIO_MODE_INPUT;               /* 输入模式 */
    gpio_config_structure_btn_decrease.pull_up_en = 1;                       /* 上拉 */
    gpio_config_structure_btn_decrease.pull_down_en = 0;                     /* 不下拉 */
    gpio_config_structure_btn_decrease.intr_type = GPIO_PIN_INTR_DISABLE;    /* 禁止中断 */

	gpio_config(&gpio_config_structure_btn_decrease);


    //初始化ADC
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_5, ADC_ATTEN_DB_11);

}
