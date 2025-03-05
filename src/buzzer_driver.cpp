#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "buzzer_driver.h"
#include "configuration.h"

#define LEDC_CHANNEL           0  // 使用通道0
#define LEDC_RESOLUTION_BITS   13 // 占空比分辨率设置为13位
#define LEDC_DUTY             (4095) // 设置占空比50%. ((2 ** 13) - 1) * 50% = 4095
#define LEDC_FREQUENCY800HZ   (800)  // 800Hz
#define LEDC_FREQUENCY2KHZ    (2000) // 2KHz
#define LEDC_FREQUENCY4KHZ    (4000) // 4KHz

QueueHandle_t bz_queue = NULL;

static void ledc_init()
{
    // 配置LEDC通道
    ledcSetup(LEDC_CHANNEL, LEDC_FREQUENCY800HZ, LEDC_RESOLUTION_BITS);
    ledcAttachPin(PIN_NUM_BZ, LEDC_CHANNEL);
    Serial.println("LEDC initialized");
}

void buzzer_task(void* parameter)
{
    unsigned char ret_bz, cmd_rev;
    bool bu_busy_flag = false;

    while (1)
    {
        ret_bz = xQueueReceive(bz_queue, &cmd_rev, 0);
        if (ret_bz)
        {
            if (!bu_busy_flag)
            {
                bu_busy_flag = true; // 置忙
                Serial.printf("Buzzer received command: %d\n", cmd_rev);

                if (cmd_rev == DDIDI) // 蜂鸣器 4KHz 短促 滴~ 滴
                {
                    ledcChangeFrequency(LEDC_CHANNEL, LEDC_FREQUENCY4KHZ, LEDC_RESOLUTION_BITS);
                    ledcWrite(LEDC_CHANNEL, LEDC_DUTY);
                    vTaskDelay(150 / portTICK_PERIOD_MS);
                    ledcWrite(LEDC_CHANNEL, 0);
                    vTaskDelay(200 / portTICK_PERIOD_MS);

                    ledcWrite(LEDC_CHANNEL, LEDC_DUTY);
                    vTaskDelay(50 / portTICK_PERIOD_MS);
                    ledcWrite(LEDC_CHANNEL, 0);
                }
                else if (cmd_rev == DIDI) // 蜂鸣器 800Hz 滴 滴
                {
                    ledcChangeFrequency(LEDC_CHANNEL, LEDC_FREQUENCY800HZ, LEDC_RESOLUTION_BITS);
                    ledcWrite(LEDC_CHANNEL, LEDC_DUTY);
                    vTaskDelay(200 / portTICK_PERIOD_MS);
                    ledcWrite(LEDC_CHANNEL, 0);
                    vTaskDelay(200 / portTICK_PERIOD_MS);

                    ledcWrite(LEDC_CHANNEL, LEDC_DUTY);
                    vTaskDelay(200 / portTICK_PERIOD_MS);
                    ledcWrite(LEDC_CHANNEL, 0);
                    vTaskDelay(500 / portTICK_PERIOD_MS);
                }
                else if (cmd_rev == DI) // 蜂鸣器 4KHz 滴
                {
                    ledcChangeFrequency(LEDC_CHANNEL, LEDC_FREQUENCY4KHZ, LEDC_RESOLUTION_BITS);
                    ledcWrite(LEDC_CHANNEL, LEDC_DUTY);
                    vTaskDelay(50 / portTICK_PERIOD_MS);
                    ledcWrite(LEDC_CHANNEL, 0);
                }
                else if (cmd_rev == CONDI) // 蜂鸣器持续响
                {
                    ledcChangeFrequency(LEDC_CHANNEL, LEDC_FREQUENCY800HZ, LEDC_RESOLUTION_BITS);
                    ledcWrite(LEDC_CHANNEL, LEDC_DUTY);
                }
                else if (cmd_rev == EXITDI) // 关闭蜂鸣器持续响
                {
                    ledcWrite(LEDC_CHANNEL, 0);
                }
                bu_busy_flag = false; // 置空闲
            }
        }
        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
}

void buzzer_init()
{
    pinMode(PIN_NUM_BZ, OUTPUT);
    digitalWrite(PIN_NUM_BZ, LOW); // 蜂鸣器控制引脚 电平置0

    // 创建消息队列
    bz_queue = xQueueCreate(1, 1);
    if (bz_queue == NULL)
    {
        Serial.println("Error: bz_queue create fail");
    }

    ledc_init();

    // 修改任务创建，buzzer_task 现在符合 TaskFunction_t 类型要求
    xTaskCreate(buzzer_task, "buzzer_task", 2048, NULL, 15, NULL);
    Serial.println("Buzzer task created");
}
