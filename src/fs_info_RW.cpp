/* File System (LittleFS) Read and Write - Implementation
 *
 * This file provides functions to read and write configuration data
 * using LittleFS instead of NVS (Non-Volatile Storage)
 */
#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "LittleFS.h"
#include "fs_info_RW.h"

static const char* TAG = "fs info R/W";

FS_INFO_E StartEinstellen_config_flag = FS_StartEinstellen_INFO_NULL;

// 配置文件路径
// const char* WIFI_CONFIG_PATH = "/wifi_config.dat";
const char* START_CONFIG_PATH = "/start_config.dat";

/* 写入开机配置 */
FS_INFO_E fs_StartEinstellen_information_write(char* inBuf, uint8_t len)
{
    if (!LittleFS.begin(false))
    {
        Serial.println("[ERROR] Failed to mount LittleFS");
        return FS_WIFI_INFO_ERROR;
    }

    File file = LittleFS.open(START_CONFIG_PATH, "w");
    if (!file)
    {
        Serial.println("[ERROR] Failed to open start config file for writing");
        return FS_WIFI_INFO_ERROR;
    }

    size_t bytesWritten = file.write((uint8_t*)inBuf, len);
    file.close();

    if (bytesWritten == len)
    {
        Serial.println("[INFO] Start config write done!");
        return FS_StartEinstellen_INFO_SAVE;
    }
    else
    {
        Serial.println("[ERROR] Start config write error!");
        return FS_WIFI_INFO_ERROR;
    }
}

/* 读取开机配置 */
FS_INFO_E fs_StartEinstellen_information_read(char* outBuf, uint8_t len)
{
    if (!LittleFS.begin(false))
    {
        Serial.println("[ERROR] Failed to mount LittleFS");
        return FS_WIFI_INFO_ERROR;
    }

    if (!LittleFS.exists(START_CONFIG_PATH))
    {
        Serial.println("[INFO] No start config file found");
        return FS_StartEinstellen_INFO_NULL;
    }

    File file = LittleFS.open(START_CONFIG_PATH, "r");
    if (!file)
    {
        Serial.println("[ERROR] Failed to open start config file for reading");
        return FS_WIFI_INFO_ERROR;
    }

    size_t bytesRead = file.read((uint8_t*)outBuf, len);
    file.close();

    if (bytesRead > 0)
    {
        Serial.println("[INFO] Has start einstellen config info");
        return FS_StartEinstellen_INFO_SAVE;
    }
    else
    {
        Serial.println("[ERROR] Start config read error!");
        return FS_StartEinstellen_INFO_NULL;
    }
}

// /* 写入WiFi信息 */
// void fs_wifi_information_write()
// {
//     // 实现WiFi信息写入
//     // 这里需要根据您的具体需求实现
//     Serial.println("[INFO] WiFi information write function called");
// }
//
// /* 读取WiFi信息 */
// FS_INFO_E fs_wifi_information_read()
// {
//     // 实现WiFi信息读取
//     // 这里需要根据您的具体需求实现
//     Serial.println("[INFO] WiFi information read function called");
//
//     if (!LittleFS.exists(WIFI_CONFIG_PATH)) {
//         Serial.println("[INFO] No WiFi config file found");
//         return FS_WIFI_INFO_NULL;
//     }
//
//     return FS_WIFI_INFO_SAVE;
// }
