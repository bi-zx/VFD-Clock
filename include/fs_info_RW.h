#ifndef _FS_INFO_RW_H_
#define _FS_INFO_RW_H_

typedef enum
{
    FS_WIFI_INFO_ERROR = 0,
    FS_WIFI_INFO_NULL,
    FS_WIFI_INFO_SAVE,
    FS_StartEinstellen_INFO_NULL,
    FS_StartEinstellen_INFO_SAVE,
} FS_INFO_E;

// WiFi配置结构体
typedef struct
{
    uint8_t ssid[32];
    uint8_t password[64];
} wifi_info_config_t;

void fs_wifi_information_write(wifi_info_config_t* config, size_t len);
FS_INFO_E fs_wifi_information_read(wifi_info_config_t* config, size_t len);
FS_INFO_E fs_StartEinstellen_information_write(char* inBuf, uint8_t len);
FS_INFO_E fs_StartEinstellen_information_read(char* outBuf, uint8_t len);

#endif
