#ifndef NTP_TIME_H
#define NTP_TIME_H

// NTP 时间同步函数
void ntp_time_get();

// 获取格式化的日期时间字符串
void getFormattedDateTime(char* dateStr, char* timeStr);

#endif // NTP_TIME_H
