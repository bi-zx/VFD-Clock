#include <Arduino.h>
#include <WiFiUdp.h>
#include <ctime>
#include <WiFi.h>
#include <NTPClient.h>
#include <sys/time.h>

// 定义 NTP 服务器
#define NTP_SERVER "ntp.aliyun.com"
#define NTP_TIMEOUT 1500

// 创建 UDP 和 NTPClient 对象
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_SERVER, 0, NTP_TIMEOUT);

// NTP 时间同步函数
void ntp_time_get()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("[ERROR] WiFi未连接，无法同步时间");
        return;
    }

    timeClient.begin();
    if (!timeClient.forceUpdate())
    {
        Serial.println("[ERROR] NTP时间更新失败");
        timeClient.end();
        return;
    }

    // 获取 Unix 时间戳
    unsigned long epochTime = timeClient.getEpochTime();

    // 设置系统时间
    struct timeval setTv;
    setTv.tv_sec = epochTime;
    setTv.tv_usec = 0; // NTPClient 不提供毫秒级精度，这里设置为0

    // 设置系统时间
    settimeofday(&setTv, NULL);

    Serial.println("[INFO] NTP时间同步成功");
    Serial.printf("[INFO] 当前时间戳: %lu\n", epochTime);

    timeClient.end();
    return;
}

// 获取格式化的日期时间字符串
void getFormattedDateTime(char* dateStr, char* timeStr)
{
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);

    // 格式化日期 "YYYY-MM-DD"
    strftime(dateStr, 11, "%Y-%m-%d", &timeinfo);
    // 格式化时间 "HH:MM:SS"
    strftime(timeStr, 9, "%H:%M:%S", &timeinfo);
}
