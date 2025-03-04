/********************************** (C) COPYRIGHT *******************************
* File Name          : 13ST84GINK.c
* Author             : Feiyu
* Version            : V1.0
* Date               : 2025/03/03
* Description        : ESP32-C3   模拟SPI
*******************************************************************************/
#include <Arduino.h>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include "configuration.h"
#include "13ST84GINK.h"

/* 用于8位数据/命令传输 */
void VFDWriteData(unsigned char dat)
{
    unsigned char mask;

    for (mask = 0x01; mask != 0; mask <<= 1)
    {
        digitalWrite(PIN_NUM_SCLK, 0);
        if (mask & dat)
            digitalWrite(PIN_NUM_MOSI, 1);
        else
            digitalWrite(PIN_NUM_MOSI, 0);
        usleep(1);
        digitalWrite(PIN_NUM_SCLK, 1);
        usleep(1);
    }
    usleep(1);
}

/* VFD电源控制 */
void VFDPowerCtrl(unsigned char set)
{
    if (set == 1)
    {
        digitalWrite(PIN_NUM_EFEN_EN, 1); //打开灯丝电源
        vTaskDelay(200 / portTICK_PERIOD_MS); //等待电压稳定
        digitalWrite(PIN_NUM_HDCDC_EN, 1); //打开高压升压
        vTaskDelay(50 / portTICK_PERIOD_MS); //等待电压稳定
    }
    else if (set == 0)
    {
        digitalWrite(PIN_NUM_HDCDC_EN, 0); //关闭高压升压
        vTaskDelay(10 / portTICK_PERIOD_MS); //等待电压稳定
        digitalWrite(PIN_NUM_EFEN_EN, 0); //关闭灯丝电源
        vTaskDelay(10 / portTICK_PERIOD_MS); //等待电压稳定
    }
}

/* 亮度设置 */
void SetLuminance(unsigned char lum)
{
    digitalWrite(PIN_NUM_CS, 0); //开始传输
    usleep(1);
    VFDWriteData(0xe4);
    VFDWriteData(lum); //亮度设置
    digitalWrite(PIN_NUM_CS, 1); //停止传输
    usleep(1);
}

/* 用于开/关 当前屏幕显示 0关  1开 */
void VFDDisplaySwitch(unsigned char set)
{
    digitalWrite(PIN_NUM_CS, 0); //开始传输
    usleep(1);
    if (set == 1)
    {
        VFDWriteData(0xe8); //打开显示 指令
    }
    else if (set == 0)
    {
        VFDWriteData(0xea); //关闭显示 指令
    }
    digitalWrite(PIN_NUM_CS, 1); //停止传输
    usleep(1);
}

/* 待机设置  0正常模式  1待机 */
void VFDDStandby(unsigned char set)
{
    digitalWrite(PIN_NUM_CS, 0); //开始传输
    usleep(1);
    if (set == 1)
    {
        VFDWriteData(0xed); //进入待机 指令
    }
    else if (set == 0)
    {
        VFDWriteData(0xec); //正常模式 指令
    }
    digitalWrite(PIN_NUM_CS, 1); //停止传输
    usleep(1);
}

/* VFD初始化**/
void VFDInit(unsigned char slum)
{
    // 使用Arduino的pinMode初始化GPIO
    pinMode(PIN_NUM_EFEN_EN, OUTPUT);
    pinMode(PIN_NUM_HDCDC_EN, OUTPUT);
    pinMode(PIN_NUM_REST, OUTPUT);
    pinMode(PIN_NUM_CS, OUTPUT);
    pinMode(PIN_NUM_MOSI, OUTPUT);
    pinMode(PIN_NUM_SCLK, OUTPUT);

    //打开电源 并开始初始化VFD屏
    VFDPowerCtrl(1); //打开VFD电源
    Serial.println("VFD power on.");
    //spi_initialize();//初始化SPI
    Serial.println("SPI init complete.");
    digitalWrite(PIN_NUM_REST, 0); //拉低REST  再拉高重启VFD内置控制芯片
    usleep(20);
    digitalWrite(PIN_NUM_REST, 1);
    vTaskDelay(10 / portTICK_PERIOD_MS); //等待VFD内置控制芯片重启好

    digitalWrite(PIN_NUM_CS, 0); //开始传输
    usleep(1);
    VFDWriteData(0xe0); //显示设置
    VFDWriteData(0x0c); //设置显示G1~G13
    digitalWrite(PIN_NUM_CS, 1); //停止传输
    usleep(1);

    //显示模式设置
    digitalWrite(PIN_NUM_CS, 0); //开始传输
    usleep(1);
    VFDWriteData(0x00); //
    VFDWriteData(0x00); //
    digitalWrite(PIN_NUM_CS, 1); //停止传输
    usleep(1);

    //设置亮度并
    SetLuminance(slum);
}

/* 写CGRAM */
void WriteCGRAM(unsigned char x, unsigned char* arr)
{
    digitalWrite(PIN_NUM_CS, 0); //CS拉低
    usleep(1);
    VFDWriteData(0x40 + x); //地址寄存器起始位置

    // 修改数组访问方式，直接写入连续的5个字节
    for (unsigned char i = 0; i < 5; i++)
    {
        VFDWriteData(arr[i]);
    }

    digitalWrite(PIN_NUM_CS, 1); //CS拉高
    usleep(1);
}

/* 写入显示指令 */
void WriteDisplayCMD()
{
    digitalWrite(PIN_NUM_CS, 0); //开始传输
    usleep(1);
    VFDWriteData(0xe8); //显示写入内容
    digitalWrite(PIN_NUM_CS, 1); //停止传输
    usleep(1);
}

/* 写入一位自定义CGRAM字符待显示 */
void VFDWriteOneDIYChar(unsigned char x, unsigned char chr)
{
    digitalWrite(PIN_NUM_CS, 0); //开始传输
    usleep(1);
    VFDWriteData(0x20 + x); //地址寄存器起始位置
    VFDWriteData(chr);
    digitalWrite(PIN_NUM_CS, 1); //停止传输
    usleep(1);
}

/* 写入并显示一位自定义CGRAM字符 */
void VFDWriteOneDIYCharAndShow(unsigned char x, unsigned char chr)
{
    digitalWrite(PIN_NUM_CS, 0); //开始传输
    usleep(1);
    VFDWriteData(0x20 + x); //地址寄存器起始位置
    VFDWriteData(chr);
    digitalWrite(PIN_NUM_CS, 1); //停止传输
    usleep(1);

    WriteDisplayCMD(); //显示写入内容
}

/* 写入字符串 */
void VFDWriteStr(unsigned char x, char* str)
{
    digitalWrite(PIN_NUM_CS, 0); //开始传输
    usleep(1);
    VFDWriteData(0x20 + x); //地址寄存器起始位置
    while (*str)
    {
        VFDWriteData(*str); //ascii与对应字符表转换
        str++;
    }
    digitalWrite(PIN_NUM_CS, 1); //停止传输
    usleep(1);
}

/* 写入字符串并显示 */
void VFDWriteStrAndShow(unsigned char x, char* str)
{
    digitalWrite(PIN_NUM_CS, 0); //开始传输
    usleep(1);
    VFDWriteData(0x20 + x); //地址寄存器起始位置
    while (*str)
    {
        VFDWriteData(*str); //ascii与对应字符表转换
        str++;
    }
    digitalWrite(PIN_NUM_CS, 1); //停止传输
    usleep(1);

    WriteDisplayCMD(); //显示写入内容
}

/* 写一个ADRAM并显示 */
void VFDWriteOneADRAMAndShow(unsigned char x, unsigned char dat)
{
    digitalWrite(PIN_NUM_CS, 0); //开始传输
    usleep(1);
    VFDWriteData(0x60 + x); //地址寄存器起始位置
    VFDWriteData(dat); //ascii与对应字符表转换
    digitalWrite(PIN_NUM_CS, 1); //停止传输
    usleep(1);

    WriteDisplayCMD(); //显示写入内容
}

/* 写ADRAM并显示 */
void VFDWriteAllADRAMAndShow(unsigned char* dat)
{
    digitalWrite(PIN_NUM_CS, 0); //开始传输
    usleep(1);
    VFDWriteData(0x60 + 0); //地址寄存器起始位置
    while (*dat)
    {
        VFDWriteData(*dat); //ascii与对应字符表转换
        dat++;
    }
    digitalWrite(PIN_NUM_CS, 1); //停止传输
    usleep(1);

    WriteDisplayCMD(); //显示写入内容
}
