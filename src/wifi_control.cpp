/* WiFi station Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <Arduino.h>
#include <WiFi.h>
#include <cstring>
#include <ArduinoJson.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "ElegantOTA.h"
#include "LittleFS.h"
#include <WebServer.h>
#include "wifi_control.h"
#include "configuration.h"
#include "fs_info_RW.h"

WebServer server(80);

const char* wifi_ssid;
const char* wifi_pswd;

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static int s_retry_num = 0;

static uint8_t sta_start_status = 0;
static uint8_t ap_start_status = 0;

static void wifi_event_handler(WiFiEvent_t event, WiFiEventInfo_t info)
{
    Serial.printf("-----------%d event\n", event);
    wifi_info_config_t wifi_config;
    // 尝试读取保存的 WiFi 配置
    if (fs_wifi_information_read(&wifi_config, sizeof(wifi_info_config_t)) == FS_WIFI_INFO_SAVE)
    {
        // 使用保存的配置
        wifi_ssid = (char*)wifi_config.ssid;
        wifi_pswd = (char*)wifi_config.password;
        Serial.println("[INFO] Using saved WiFi configuration");
    }

    switch (event)
    {
    case ARDUINO_EVENT_WIFI_STA_START:
        Serial.println("Station Mode Started");
        WiFi.begin(wifi_ssid, wifi_pswd);
        break;

    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
        Serial.println("Connected to access point");
        break;

    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
        Serial.println("Disconnected from WiFi access point");
        if (s_retry_num < CONFIG_ESP_MAXIMUM_RETRY)
        {
            WiFi.begin(wifi_ssid, wifi_pswd);
            s_retry_num++;
            Serial.printf("Retrying to connect... (%d/%d)\n",
                          s_retry_num, CONFIG_ESP_MAXIMUM_RETRY);
        }
        else
        {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        break;

    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
        Serial.print("Got IP: ");
        Serial.println(WiFi.localIP());
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        break;

    case ARDUINO_EVENT_WIFI_AP_START:
        Serial.println("AP Mode Started");
        break;

    case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
        Serial.print("Station connected to AP: ");
        Serial.println(WiFi.softAPgetStationNum());
        break;

    case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
        Serial.print("Station disconnected from AP: ");
        Serial.println(WiFi.softAPgetStationNum());
        break;

    default:
        break;
    }
}

void ota_init()
{
    // 基础网页
    server.on("/", HTTP_GET, []()
    {
        File file = LittleFS.open("/index.html", "r");
        if (file)
        {
            server.streamFile(file, "text/html");
            file.close();
        }
    });
    // 添加保存 WiFi 配置的路由
    server.on("/save-wifi", HTTP_POST, []()
    {
        String data = server.arg("plain");
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, data);

        if (error)
        {
            server.send(400, "text/plain", "JSON解析错误");
            return;
        }

        const char* ssid = doc["ssid"];
        const char* password = doc["password"];

        // 验证 SSID（必须有值）
        if (!ssid || strlen(ssid) == 0 || strlen(ssid) > 31)
        {
            server.send(400, "text/plain", "SSID 无效");
            return;
        }

        // 验证密码（允许为空，但如果有值则不能超过63字符）
        if (password && strlen(password) > 63)
        {
            server.send(400, "text/plain", "密码无效");
            return;
        }

        // 创建配置结构体
        wifi_info_config_t config;
        memset(&config, 0, sizeof(wifi_info_config_t));
        strlcpy((char*)config.ssid, ssid, sizeof(config.ssid));
        if (password)
        {
            strlcpy((char*)config.password, password, sizeof(config.password));
        }
        else
        {
            config.password[0] = '\0'; // 确保密码为空字符串
        }
        // 保存到文件系统
        fs_wifi_information_write(&config, sizeof(wifi_info_config_t));
        // 发送成功响应，并通知前端将要重启
        server.send(200, "text/plain", "WiFi配置保存成功！设备将在3秒后重启...");

        // 延迟3秒后重启，确保响应能够发送到客户端
        delay(3000);
        ESP.restart();
    });
    // 启动 ElegantOTA
    ElegantOTA.begin(&server);

    // 开启服务器
    server.begin();
    Serial.println("[INFO] HTTP server started");
}

void wifi_init_sta()
{
    wifi_info_config_t wifi_config;
    // 尝试读取保存的 WiFi 配置
    FS_INFO_E read_result = fs_wifi_information_read(&wifi_config, sizeof(wifi_info_config_t));

    if (read_result == FS_WIFI_INFO_SAVE)
    {
        // 验证配置有效性
        if (strlen((char*)wifi_config.ssid) == 0 || strlen((char*)wifi_config.ssid) > 31)
        {
            Serial.println("[ERROR] Stored SSID invalid, using default configuration");
            wifi_ssid = AP_SSID;
            wifi_pswd = AP_PASSWORD;
        }
        else
        {
            // 使用保存的配置
            wifi_ssid = (char*)wifi_config.ssid;
            wifi_pswd = (char*)wifi_config.password;
            Serial.println("[INFO] Using saved WiFi configuration");
        }
    }
    else
    {
        Serial.println("[INFO] Using default WiFi configuration");
        wifi_ssid = AP_SSID;
        wifi_pswd = AP_PASSWORD;
    }
    s_wifi_event_group = xEventGroupCreate();
    WiFi.onEvent(wifi_event_handler);
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifi_ssid, wifi_pswd);

    Serial.println("wifi_init_sta finished.");

    // 设置较短的超时时间（10秒）
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           10000 / portTICK_PERIOD_MS);

    if (bits & WIFI_CONNECTED_BIT)
    {
        Serial.printf("connected to ap SSID:%s password:%s\n",
                      wifi_ssid, wifi_pswd);
    }
    else
    {
        Serial.printf("Failed to connect to SSID:%s, password:%s\n",
                      wifi_ssid, wifi_pswd);
        // 连接失败，停止 STA 模式
        wifi_sta_stop();
    }
}

void wifi_init_ap()
{
    // 注册WiFi事件处理函数
    WiFi.onEvent(wifi_event_handler);

    // 设置WiFi模式为AP
    WiFi.mode(WIFI_AP);

    // 配置AP模式
    bool result = WiFi.softAP(AP_SSID, AP_PASSWORD, 1, 0, AP_MAX_CONN);

    if (result)
    {
        Serial.println("AP Mode Configured Successfully");
        Serial.print("AP IP address: ");
        Serial.println(WiFi.softAPIP());
        Serial.printf("wifi_init_ap finished.\nSSID:%s PASSWORD:%s\n", AP_SSID, AP_PASSWORD);
    }
    else
    {
        Serial.println("AP Mode Failed!");
    }
}

void wifi_sta_start()
{
    if (sta_start_status == 0)
    {
        sta_start_status = 1;
        Serial.println("ESP_WIFI_MODE_STA START");
        wifi_init_sta();
    }
    else
    {
        Serial.println("ESP_WIFI_MODE_STA IS STARTING");
    }
}

void wifi_sta_stop()
{
    if (sta_start_status == 1)
    {
        sta_start_status = 0;
        Serial.println("ESP_WIFI_MODE_STA STOP");
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
        vEventGroupDelete(s_wifi_event_group);
    }
    else
    {
        Serial.println("ESP_WIFI_MODE_STA IS STOPPING");
    }
}

void wifi_ap_start()
{
    if (ap_start_status == 0)
    {
        ap_start_status = 1;
        Serial.println("ESP_WIFI_MODE_AP START");
        wifi_init_ap();
    }
    else
    {
        Serial.println("ESP_WIFI_MODE_AP IS STARTING");
    }
}

void wifi_ap_stop()
{
    if (ap_start_status == 1)
    {
        ap_start_status = 0;
        Serial.println("ESP_WIFI_MODE_AP STOP");
        WiFi.softAPdisconnect(true);
        WiFi.mode(WIFI_OFF);
    }
    else
    {
        Serial.println("ESP_WIFI_MODE_AP IS STOPPING");
    }
}

// void wifi_disconnect()
// {
//     WiFi.disconnect();
// }
//
// void wifi_connect()
// {
//     WiFi.begin(wifi_ssid, wifi_pswd);
// }
