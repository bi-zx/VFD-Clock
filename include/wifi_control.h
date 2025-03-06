#ifndef WIFI_CONTROL_H
#define WIFI_CONTROL_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

extern WebServer server;

// OTA 初始化函数
void ota_init();

// WiFi Station 模式函数
void wifi_sta_start();
void wifi_sta_stop();
void wifi_disconnect();
void wifi_connect();

// WiFi AP 模式函数
void wifi_ap_start();
void wifi_ap_stop();

#endif // WIFI_CONTROL_H
