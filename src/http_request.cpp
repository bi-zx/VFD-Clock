// #include "esp_system.h"
// #include "esp_wifi.h"
// #include "esp_event.h"
// #include "esp_log.h"
//
// #include "lwip/err.h"
// #include "lwip/sockets.h"
// #include "lwip/sys.h"
// #include "lwip/netdb.h"
// #include "lwip/dns.h"
//
// #include "esp_netif.h"
// #include "esp_tls.h"
// #include "esp_http_client.h"
// #include "cJSON.h"

/* HTTP GET Example using Arduino HTTP Client
 *
 * This example code is in the Public Domain (or CC0 licensed, at your option.)
 *
 * Unless required by applicable law or agreed to in writing, this
 * software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.
 */
#include <Arduino.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <HTTPClient.h>
#include <WiFi.h>
#include <ArduinoJson.h>

#include "http_request.h"
#include <time.h>
#include <sys/time.h>

static const char* TAG = "HTTP_CLIENT";

//https://acs.m.taobao.com/gw/mtop.common.getTimestamp/ //美团获取时间api接口2
//https://cube.meituan.com/ipromotion/cube/toc/component/base/getServerCurrentTime //美团获取时间api接口
// 淘宝获取时间API接口
#define TIME_URL "http://acs.m.taobao.com/gw/mtop.common.getTimestamp/"
// B站粉丝数API接口
#define FANS_URL "https://api.bilibili.com/x/relation/stat?vmid=59395298&jsonp=jsonp"

bool set_time_flag = 0;
int fans_type = 0;

/* 字符串转整形 */
unsigned int stringToint(char source[], unsigned char startBit, unsigned char len)
{
    unsigned char i;
    unsigned int temp1, temp2;
    temp2 = 0;
    for (i = startBit; i < len; i++)
    {
        temp1 = source[i] - '0';
        temp2 = temp2 * 10;
        temp2 += temp1;
    }

    return temp2;
}

/* 解析时间JSON数据 */
static void parse_time_json(String payload)
{
    // 为ArduinoJson分配内存
    DynamicJsonDocument doc(1024);

    // 解析JSON
    DeserializationError error = deserializeJson(doc, payload);

    if (error)
    {
        Serial.print("[ERROR] JSON解析失败: ");
        Serial.println(error.c_str());
        return;
    }

    // 提取时间戳
    if (doc.containsKey("data") && doc["data"].containsKey("t"))
    {
        Serial.println("[INFO] 开始解析JSON...");
        const char* timestamp = doc["data"]["t"];
        Serial.print("[INFO] 系统时间: ");
        Serial.println(timestamp);

        // 设置系统时间
        struct timeval setTv;
        char time[20];
        strcpy(time, timestamp);

        setTv.tv_sec = stringToint(time, 0, 10); // 从time字符串数据中提取出时间戳 单位秒，后3位毫秒舍弃
        setTv.tv_usec = stringToint(time, 10, 3); // 从time字符串数据中提取出时间戳 单位毫秒，前10位秒舍弃
        setTv.tv_usec = setTv.tv_usec + 40; // 补偿时间差
        setTv.tv_usec = setTv.tv_usec * 1000; // 把毫秒转微秒
        settimeofday(&setTv, 0); // 设置时间

        Serial.println("[INFO] 获取时间完成!");
        set_time_flag = 1;
    }
    else
    {
        Serial.println("[ERROR] JSON数据格式不正确");
    }
}

/* HTTP获取时间 */
void http_time_get()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("[ERROR] WiFi未连接");
        return;
    }

    HTTPClient http;
    Serial.print("[INFO] 正在连接到: ");
    Serial.println(TIME_URL);

    // 开始连接
    http.begin(TIME_URL);

    // 发送GET请求
    int httpCode = http.GET();

    // 检查返回码
    if (httpCode > 0)
    {
        Serial.printf("[INFO] HTTP状态码: %d\n", httpCode);

        // 如果请求成功
        if (httpCode == HTTP_CODE_OK)
        {
            String payload = http.getString();
            Serial.printf("[INFO] 接收数据长度: %d\n", payload.length());

            // 解析时间数据
            if (payload.length() < 110)
            {
                parse_time_json(payload);
            }
        }
    }
    else
    {
        Serial.printf("[ERROR] HTTP请求失败, 错误: %s\n", http.errorToString(httpCode).c_str());
    }

    // 关闭连接
    http.end();
}

QueueHandle_t http_get_event_queue;

/* 设置HTTP请求类型 */
void http_set_type(HTTP_GET_TYPE_E type)
{
    HTTP_GET_EVENT_T evt;
    evt.type = type;
    xQueueSend(http_get_event_queue, &evt, 10);
}

/* HTTP请求任务 */
static void http_get_task(void* pvParameters)
{
    HTTP_GET_EVENT_T evt;

    while (1)
    {
        // 检查队列中是否有新的请求类型
        if (xQueueReceive(http_get_event_queue, &evt, 0) == pdTRUE)
        {
            switch (evt.type)
            {
            case HTTP_GET_TIME:
                Serial.println("[INFO] 执行时间获取请求");
                http_time_get();
                break;
            case HTTP_GET_WEATHER:
                Serial.println("[INFO] 执行天气获取请求");
            // 实现天气获取功能
                break;
            case HTTP_GET_FANS:
                Serial.println("[INFO] 执行粉丝数获取请求");
            // 实现粉丝数获取功能
                break;
            case HTTP_GET_CITY:
                Serial.println("[INFO] 执行城市获取请求");
            // 实现城市获取功能
                break;
            default:
                break;
            }
        }

        // 定期检查时间同步状态
        if (set_time_flag == 1)
        {
            set_time_flag = 0;
            Serial.println("[INFO] 时间同步已完成");
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    vTaskDelete(NULL);
    Serial.println("[INFO] HTTP获取任务已关闭");
}

/* 初始化HTTP请求模块 */
void http_request_init()
{
    Serial.println("[INFO] 初始化HTTP请求模块");
    http_get_event_queue = xQueueCreate(10, sizeof(HTTP_GET_EVENT_T));
    if (http_get_event_queue == NULL)
    {
        Serial.println("[ERROR] 创建HTTP请求队列失败");
        return;
    }

    xTaskCreate(
        http_get_task, // 任务函数
        "http_get_task", // 任务名称
        4096, // 堆栈大小
        NULL, // 任务参数
        5, // 任务优先级
        NULL // 任务句柄
    );

    Serial.println("[INFO] HTTP请求模块初始化完成");
}
