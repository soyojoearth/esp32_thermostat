#ifndef __GLOBAL__
#define __GLOBAL__  "__GLOBAL__"

#define adc_io 33 //ADC输入口

#define beeper_io 23 //蜂鸣器IO

#define led 2 //LED指示灯，系统

#define btn 14 //按钮

#define btn_temp 12 //按钮_温标切换
#define btn_decrease 25 //按钮_减小
#define btn_increase 26 //按钮_增加


#define TM1640_CLK 19
#define TM1640_DATA 18


#define relay_heat_io 16//制热口，右
#define relay_cool_io 17//制冷口，左


extern int runningTime;//系统开机时间累计，单位：秒
extern int countWifiConnected;//系统开机后WiFi连接成功的次数
extern int countMqttConnected;//系统开机后Mqtt连接成功的次数


extern int wifiMode;
extern int power;//开机默认灯亮

extern int mode_setting;//0目标温度，1回差值，2修正值，3高温警报值，4低温警报值
//以下值，全部放大10倍
extern int tempTarget;//目标值
extern int tempDiff;//回差值
extern int tempCal;//修正值
extern int tempAlarmH;//高温警报值
extern int tempAlarmL;//低温警报值
extern int coolingDelay;//cooling Delay
extern int heatingDelay;//heating Delay

extern int tempUnit;//0摄氏度 1华氏度

void initIO();


#endif
