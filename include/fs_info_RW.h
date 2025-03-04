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

// void fs_wifi_information_write();
// FS_INFO_E fs_wifi_information_read();
FS_INFO_E fs_StartEinstellen_information_write(char* inBuf, uint8_t len);
FS_INFO_E fs_StartEinstellen_information_read(char* outBuf, uint8_t len);

#endif
