#ifndef WIFI_CONTROL_H
#define WIFI_CONTROL_H

#include <Arduino.h>
#include <WiFi.h>

// WiFi Station 模式函数
void wifi_sta_start(void);
void wifi_sta_stop(void);
void wifi_disconnect(void);
void wifi_connect(void);

// WiFi AP 模式函数
void wifi_ap_start(void);
void wifi_ap_stop(void);

#endif // WIFI_CONTROL_H