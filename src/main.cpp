#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "LittleFS.h"
#include "clock_function.h"
#include "13ST84GINK.h"

#include "measuring_lightIntensity.h"
#include "buzzer_driver.h"
#include "key_driver.h"
#include "fs_info_RW.h"

void clock_funtion_task(void* parameter)
{
    VFDWriteStrAndShow(1, "Hello,World!");
    ADCInit();
    buzzer_init();
    // key_init();
    while (1)
    {
        unsigned char luminance = measure_brightness();
        // Serial.println(luminance);
        SetLuminance(luminance);

        // unsigned char bz_dat = DDIDI;
        // xQueueSend(bz_queue, &bz_dat, 0); //给蜂鸣器任务
        // vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

void setup()
{
    Serial.begin(115200);

    // 初始化LittleFS
    if (!LittleFS.begin(true))
    {
        Serial.println("An Error has occurred while mounting LittleFS");
    }
    // 初始化VFD
    VFDInit(0x30);

    // 创建主任务
    xTaskCreate(
        clock_funtion_task, // 任务函数
        "clock_funtion", // 任务名称
        8192, // 堆栈大小
        NULL, // 任务参数
        5, // 任务优先级
        NULL // 任务句柄
    );
    Serial.println("Start up completed.");
}

void loop()
{
}
