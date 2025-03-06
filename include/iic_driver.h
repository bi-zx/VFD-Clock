#ifndef IIC_DRIVER_H
#define IIC_DRIVER_H


// 初始化 IIC总线
void iic_init();

// 读取温湿度数据
bool aht10_read(float* temperature, float* humidity);

// RTC相关函数
bool rtc_time_get(); // 从RTC读取时间并设置系统时间
bool rtc_time_set(); // 从系统时间更新RTC时间

#endif // IIC_DRIVER_H
