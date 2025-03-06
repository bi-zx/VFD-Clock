#include <Arduino.h>
#include <Wire.h>
#include "Adafruit_AHTX0.h"
#include "RTClib.h"
#include "configuration.h"
#include "iic_driver.h"


static Adafruit_AHTX0 AHT10;
static RTC_DS3231 rtc;
static bool rtc_initialized = false;

void iic_init()
{
    Wire.begin(PIN_NUM_IIC_SDA, PIN_NUM_IIC_SDL);

    // 初始化 AHT10
    if (!AHT10.begin(&Wire))
    {
        Serial.println("[ERROR] AHT10 初始化失败");
    }
    else
    {
        Serial.println("[INFO] AHT10 初始化成功");
    }

    // 初始化 RTC
    if (!rtc.begin(&Wire))
    {
        Serial.println("[ERROR] RTC 初始化失败");
    }
    else
    {
        rtc_initialized = true;
        Serial.println("[INFO] RTC 初始化成功");

        // if (rtc.lostPower())
        // {
        //     Serial.println("[WARN] RTC 掉电，需要重新设置时间");
        //     rtc_time_set(); // 使用系统时间初始化 RTC
        // }
    }
}

bool aht10_read(float* temperature, float* humidity)
{
    sensors_event_t humidity_event, temp_event;

    if (!AHT10.getEvent(&humidity_event, &temp_event))
    {
        // Serial.println("[ERROR] AHT10 读取失败");
        return false;
    }

    *temperature = temp_event.temperature;
    *humidity = humidity_event.relative_humidity;
    return true;
}

bool rtc_time_get()
{
    if (!rtc_initialized)
    {
        Serial.println("[ERROR] RTC 未初始化");
        return false;
    }

    DateTime now = rtc.now();

    // 设置系统时间
    struct timeval tv;
    struct tm timeinfo;

    timeinfo.tm_year = now.year() - 1900;
    timeinfo.tm_mon = now.month() - 1;
    timeinfo.tm_mday = now.day();
    timeinfo.tm_hour = now.hour();
    timeinfo.tm_min = now.minute();
    timeinfo.tm_sec = now.second();

    tv.tv_sec = mktime(&timeinfo);
    tv.tv_usec = 0;

    if (settimeofday(&tv, NULL) != 0)
    {
        Serial.println("[ERROR] 设置系统时间失败");
        return false;
    }

    Serial.printf("[INFO] RTC时间同步成功: %04d-%02d-%02d %02d:%02d:%02d\n",
                  now.year(), now.month(), now.day(),
                  now.hour(), now.minute(), now.second());
    return true;
}

bool rtc_time_set()
{
    if (!rtc_initialized)
    {
        Serial.println("[ERROR] RTC 未初始化");
        return false;
    }

    time_t now;
    struct tm timeinfo;

    time(&now);
    localtime_r(&now, &timeinfo);

    rtc.adjust(DateTime(
        timeinfo.tm_year + 1900,
        timeinfo.tm_mon + 1,
        timeinfo.tm_mday,
        timeinfo.tm_hour,
        timeinfo.tm_min,
        timeinfo.tm_sec
    ));

    Serial.printf("[INFO] RTC时间已更新: %04d-%02d-%02d %02d:%02d:%02d\n",
                  timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                  timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    return true;
}
