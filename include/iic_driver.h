#ifndef IIC_DRIVER_H
#define IIC_DRIVER_H

#include <Arduino.h>

// 初始化 IIC总线
void iic_init();

// 读取温湿度数据
bool aht10_read(float* temperature, float* humidity);

#endif // IIC_DRIVER_H
