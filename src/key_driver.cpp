#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "key_driver.h"
#include "configuration.h"
#include "clock_function.h"
#include <OneButton.h>

// 按键配置
#define CLICKSPEED 200      // 单击检测时间
#define DEBOUNCETICKS 30    // 去抖时间
#define LONGPRESSTIME 800   // 长按检测时间，单位ms

// 创建按键对象
OneButton button1(key1, true, true); // 启用内部上拉，启用并联电容模式
OneButton button2(key2, true, true);
OneButton button3(key3, true, true);

// 按键检测任务
static void key_check_task(void* parameter)
{
    while (1)
    {
        button1.tick();
        button2.tick();
        button3.tick();
        vTaskDelay(5 / portTICK_PERIOD_MS); // 降低到5ms检测间隔
    }
}

void key_init()
{
    // 设置按键参数
    button1.setClickMs(CLICKSPEED);
    button1.setDebounceMs(DEBOUNCETICKS);
    button1.setPressMs(LONGPRESSTIME);

    button2.setClickMs(CLICKSPEED);
    button2.setDebounceMs(DEBOUNCETICKS);
    button2.setPressMs(LONGPRESSTIME);

    button3.setClickMs(CLICKSPEED);
    button3.setDebounceMs(DEBOUNCETICKS);
    button3.setPressMs(LONGPRESSTIME);

    // 配置按键1回调函数
    button1.attachClick([]()
    {
        xEventGroupSetBits(KeyEventHandle, key1_event);
    });
    button1.attachLongPressStart([]()
    {
        xEventGroupSetBits(KeyEventHandle, key1_long_press_event);
    });

    // 配置按键2回调函数
    button2.attachClick([]()
    {
        xEventGroupSetBits(KeyEventHandle, key2_event);
    });
    button2.attachLongPressStart([]()
    {
        xEventGroupSetBits(KeyEventHandle, key2_long_press_event);
    });

    // 配置按键3回调函数
    button3.attachClick([]()
    {
        xEventGroupSetBits(KeyEventHandle, key3_event);
    });
    button3.attachLongPressStart([]()
    {
        xEventGroupSetBits(KeyEventHandle, key3_long_press_event);
    });

    // 创建按键检测任务
    xTaskCreate(
        key_check_task,
        "key_check_task",
        2048,
        NULL,
        20,
        NULL
    );

    Serial.println("Key initialization completed");
}

void key_tick()
{
    button1.tick();
    button2.tick();
    button3.tick();
}
