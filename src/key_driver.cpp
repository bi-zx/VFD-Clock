#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "key_driver.h"
#include "configuration.h"
#include "clock_function.h"

static const char* TAG = "key driver";
QueueHandle_t keyEvenqueue = NULL; // 按键中断消息队列句柄

// 为每个按键创建独立的中断处理函数
static void IRAM_ATTR key1_intrhandle()
{
    detachInterrupt(key1);
    uint32_t key_num = key1;
    xQueueSendFromISR(keyEvenqueue, &key_num, NULL);
}

static void IRAM_ATTR key2_intrhandle()
{
    detachInterrupt(key2);
    uint32_t key_num = key2;
    xQueueSendFromISR(keyEvenqueue, &key_num, NULL);
}

static void IRAM_ATTR key3_intrhandle()
{
    detachInterrupt(key3);
    uint32_t key_num = key3;
    xQueueSendFromISR(keyEvenqueue, &key_num, NULL);
}

// 接收按键中断信息任务
static void key_intr_recv_task(void* parameter)
{
    uint32_t keyrEev;

    while (1)
    {
        if (xQueueReceive(keyEvenqueue, &keyrEev, portMAX_DELAY))
        {
            if (digitalRead(keyrEev) == LOW)
            {
                // 要做的任务
                if (keyrEev == key1) xEventGroupSetBits(KeyEventHandle, key1_event);
                if (keyrEev == key2) xEventGroupSetBits(KeyEventHandle, key2_event);
                if (keyrEev == key3) xEventGroupSetBits(KeyEventHandle, key3_event);
            }
            // 重新启用中断，根据按键选择对应的处理函数
            if (keyrEev == key1) attachInterrupt(digitalPinToInterrupt(key1), key1_intrhandle, FALLING);
            if (keyrEev == key2) attachInterrupt(digitalPinToInterrupt(key2), key2_intrhandle, FALLING);
            if (keyrEev == key3) attachInterrupt(digitalPinToInterrupt(key3), key3_intrhandle, FALLING);
        }
        vTaskDelay(300 / portTICK_PERIOD_MS);
    }
}

void key_init()
{
    // 配置GPIO
    pinMode(key1, INPUT_PULLUP);
    pinMode(key2, INPUT_PULLUP);
    pinMode(key3, INPUT_PULLUP);

    // 配置中断，每个按键使用独立的处理函数
    attachInterrupt(digitalPinToInterrupt(key1), key1_intrhandle, FALLING);
    attachInterrupt(digitalPinToInterrupt(key2), key2_intrhandle, FALLING);
    attachInterrupt(digitalPinToInterrupt(key3), key3_intrhandle, FALLING);

    // 创建消息队列以及消息接收任务
    keyEvenqueue = xQueueCreate(10, sizeof(uint32_t));
    if (keyEvenqueue == NULL)
    {
        Serial.println("Error: create queue fail!");
    }
    else
    {
        xTaskCreate(key_intr_recv_task, "key_intr_recv_task", 1024, NULL, 20, NULL);
    }

    Serial.println("Key initialization completed");
}
