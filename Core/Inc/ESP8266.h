#ifndef __ESP8266_H__
#define __ESP8266_H__

#include "usart.h" 
#include "string.h"
#include "stdio.h"
#include "stdarg.h"

#define SERVER_DOMAIN   "1420873578-gitigqkrrz.ap-guangzhou.tencentscf.com" 
#define WIFI_SSID       "1" // 替换为你的WiFi名称
#define WIFI_PWD        "12345678"  // 替换为你的WiFi密码
#define HTTP_PATH       "/"




uint8_t ESP8266_SendCmd(char *cmd, char *ack, uint32_t timeout);
void ESP8266_Init_WiFi(void);
void Upload_Image_To_Cloud(uint8_t *img_ptr, uint32_t img_size);
uint32_t Encode_Base64(uint8_t *src, uint32_t src_len, char *out) ;
uint32_t Pack_All_Camera_Data(void);

#endif
