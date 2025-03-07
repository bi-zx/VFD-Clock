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

FS_INFO_E StartEinstellen_config_flag = FS_StartEinstellen_INFO_NULL;

// 配置文件路径
const char* WIFI_CONFIG_PATH = "/wifi_config.json";
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

void fs_wifi_information_write(wifi_info_config_t* config, size_t len)
{
    // 验证输入参数
    if (!config || len != sizeof(wifi_info_config_t))
    {
        Serial.println("[ERROR] Invalid config parameters");
        return;
    }

    // 验证 SSID（必须有值）
    if (strlen((char*)config->ssid) == 0 || strlen((char*)config->ssid) > 31)
    {
        Serial.println("[ERROR] Invalid SSID length");
        return;
    }

    // 验证密码（允许为空，但如果有值则不能超过63字符）
    if (strlen((char*)config->password) > 63)
    {
        Serial.println("[ERROR] Password too long");
        return;
    }
    if (!LittleFS.begin(false))
    {
        Serial.println("[ERROR] Failed to mount LittleFS");
        return;
    }

    File file = LittleFS.open(WIFI_CONFIG_PATH, "w");
    if (!file)
    {
        Serial.println("[ERROR] Failed to open wifi config file for writing");
        return;
    }

    size_t bytesWritten = file.write((uint8_t*)config, len);
    file.close();

    if (bytesWritten == len)
    {
        Serial.println("[INFO] WiFi config write done!");
        Serial.printf("[INFO] Saved SSID: %s\n", config->ssid);
    }
    else
    {
        Serial.println("[ERROR] WiFi config write error!");
    }
}

FS_INFO_E fs_wifi_information_read(wifi_info_config_t* config, size_t len)
{
    if (!LittleFS.begin(false))
    {
        Serial.println("[ERROR] Failed to mount LittleFS");
        return FS_WIFI_INFO_ERROR;
    }

    if (!LittleFS.exists(WIFI_CONFIG_PATH))
    {
        Serial.println("[INFO] No WiFi config file found");
        return FS_WIFI_INFO_NULL;
    }

    File file = LittleFS.open(WIFI_CONFIG_PATH, "r");
    if (!file)
    {
        Serial.println("[ERROR] Failed to open wifi config file for reading");
        return FS_WIFI_INFO_ERROR;
    }

    size_t bytesRead = file.read((uint8_t*)config, len);
    file.close();

    if (bytesRead == len)
    {
        Serial.println("[INFO] WiFi config read done!");
        return FS_WIFI_INFO_SAVE;
    }
    else
    {
        Serial.println("[ERROR] WiFi config read error!");
        return FS_WIFI_INFO_NULL;
    }
}
