#include <Arduino.h>
#include <cstdio>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "measuring_lightIntensity.h"
#include "configuration.h"

static const char* TAG = "measuring lightIntensity";
#define DEFAULT_VREF    1100        // 使用默认参考电压
#define NO_OF_SAMPLES   5          // 多重采样次数

/* ADC初始化 */
void ADCInit()
{
    // 配置ADC
    analogReadResolution(12); // ADC配置为12位
    analogSetAttenuation(ADC_11db); // 设置衰减为11dB(量程0-2600mv)
    Serial.println("ADC init completed");
}

/* 读取光敏电阻分压得出亮度 */
unsigned char measure_brightness()
{
    uint32_t adc_reading = 0;
    unsigned int temp;

    // 多次测量取平均值
    for (int i = 0; i < NO_OF_SAMPLES; i++)
    {
        adc_reading += analogRead(PIN_NUM_LIGHT_SENSOR); // 读取IO模拟值
        vTaskDelay(1); // 短暂延时确保读数稳定
    }
    adc_reading /= NO_OF_SAMPLES;

    // 数据转换成电压 单位mv   测量值800-2900mv之间
    uint32_t voltage = map(adc_reading, 0, 4095, 0, 3300); // 将ADC值映射到电压值(0-3.3V)
    Serial.printf("ADC voltage: %ld mV\n", voltage);

    // 转成VFD亮度设置值   范围0-240
    temp = (uint16_t)voltage; // 截取低16位值用于计算
    if (temp < 800) temp = 800; // 测量值太低 补偿到800mv 不然屏太暗
    if (temp >= 2400) temp = 2400; // 测量值太高 补偿到2400mv
    temp = temp / 10; // VFD屏亮度分240级

    return (unsigned char)temp;
}
