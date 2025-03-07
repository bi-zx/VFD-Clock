#ifndef _configuration_H
#define _configuration_H

// STA模式连接WIFI失败时最大重试次数
#define CONFIG_ESP_MAXIMUM_RETRY 3

#define AP_SSID     "VFD-Clock"    // AP模式的SSID
#define AP_PASSWORD "12345678"     // AP模式的密码
#define AP_MAX_CONN 4              // AP模式最大连接数

// 多少点校时 的时间
#define TCALIBRATION 01 // 凌晨1点

// 时区设置
#define UTC8 10 //+8时区
// 进入夜间待机的时间以及退出时间
#define STANDBYHOUR 23   // 时 23点
#define STANDBYMIN 0     // 分 0分
#define OUTSTANDBYHOUR 7 // 时 7点
#define OUTSTANDBYMIN 0  // 分 0分

// VFD屏IO分配
#define PIN_NUM_EFEN_EN 0  // VFD灯丝电源控制
#define PIN_NUM_HDCDC_EN 1 // VFD高压升压电源控制
#define PIN_NUM_REST 8     // VFD-REST
#define PIN_NUM_MOSI 10    // VFD-SDIN
#define PIN_NUM_SCLK 20    // VFD-SCLK
#define PIN_NUM_CS 21      // VFD-CS

// 蜂鸣器IO分配
#define PIN_NUM_BZ 3

// 环境光传感器IO分配
#define PIN_NUM_LIGHT_SENSOR 4

// IIC总线IO分配
#define PIN_NUM_IIC_SDA 7
#define PIN_NUM_IIC_SDL 2

// 按键IO分配
#define key1 9
#define key2 6
#define key3 5

// 事件定义
#define key1_event 0x01
#define key2_event 0x02
#define key3_event 0x04
#define key1_long_press_event 0x08
#define key2_long_press_event 0x10
#define key3_long_press_event 0x20
#define time_calibration_event 0x40
#endif
