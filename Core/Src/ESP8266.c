#include "ESP8266.h"
#include "Radar.h"

extern int8_t fall_score,normal_score;
extern char b64_buffer[30000];
extern uint8_t test_jpeg_buffer[7500];

// 假设你已经定义了 4 个摄像头的原始缓冲区
extern uint8_t cam1_buf[];

// 用于接收 ESP8266 应答的缓存
char esp_rx_buf[256];

// 标记 DMA 是否发送完成的标志位
volatile uint8_t uart_tx_complete = 1; 

int fputc(int ch, FILE *f)
{
    uint8_t c = ch;
    HAL_UART_Transmit(&huart3, &c, 1, 100);
    return ch;
}

/**
 * @brief  向ESP8266发送AT指令并等待预期响应
 * @param  cmd: AT指令字符串
 * @param  ack: 期望收到的字符串 (例如 "OK" 或 ">")
 * @param  timeout: 超时时间(ms)
 * @retval 1:成功 0:超时或失败
 */
uint8_t ESP8266_SendCmd(char *cmd, char *ack, uint32_t timeout)
{
    uint32_t tickstart = HAL_GetTick();
    
    // 1. 【关键】发送前先等一小会，并强制清空串口接收缓冲区
    HAL_Delay(100); 
    __HAL_UART_FLUSH_DRREGISTER(&huart4); // 清除残留数据
    
    // 2. 发送指令
    HAL_UART_Transmit(&huart4, (uint8_t *)cmd, strlen(cmd), 1000);

    // 3. 循环接收并匹配响应
    memset(esp_rx_buf, 0, sizeof(esp_rx_buf));
    uint16_t rx_len = 0;
    uint8_t rx_byte;

    while ((HAL_GetTick() - tickstart) < timeout)
    {
        if (HAL_UART_Receive(&huart4, &rx_byte, 1, 5) == HAL_OK)
        {
            if (rx_len < sizeof(esp_rx_buf) - 1) esp_rx_buf[rx_len++] = rx_byte;
            if (strstr(esp_rx_buf, ack) != NULL) return 1; 
        }
    }
    return 0; 
}

//初始化
void ESP8266_Init_WiFi(void)
{
    char cmd[128];
    printf("--- 开始强制唤醒 ESP8266 ---\r\n");
	HAL_UART_Transmit(&huart4, (uint8_t*)"+++", 3, 1000);
    HAL_Delay(1000); // 必须等一秒
    
    // 2. 尝试关闭透传模式指令
    ESP8266_SendCmd("AT+CIPMODE=0\r\n", "OK", 1000);
    // 1. 复位ESP8266
	printf("复位ESP8266...\r\n");
    ESP8266_SendCmd("AT+RST\r\n", "ready", 3000);
    HAL_Delay(1000);

    // 2. 设置为Station模式
	printf("设置Station模式...\r\n");
    ESP8266_SendCmd("AT+CWMODE=1\r\n", "OK", 1500);

    // 3. 连接WiFi
	printf("连接WiFi...\r\n");
    sprintf(cmd, "AT+CWJAP=\"%s\",\"%s\"\r\n", WIFI_SSID, WIFI_PWD);
    // 连WiFi可能比较慢，给10秒超时
	
    (ESP8266_SendCmd(cmd, "WIFI GOT IP", 10000));
}

/**
 * @brief  将图片通过HTTP POST上传到腾讯云
 * @param  img_ptr: 存放JPEG图片的数组指针
 * @param  img_size: 图片的实际字节大小
 */
void Upload_Image_To_Cloud(uint8_t *img_ptr, uint32_t img_size)
{
    char cmd_buf[256];
    char http_header[512];
    
    printf("\r\n--- 启动上传流程 (大图模式) ---\r\n");
    printf("传入函数的原始图片大小: %lu 字节\r\n", (unsigned long)img_size);
    // 退出透传并清理 (保持你原来的代码不变)
    HAL_UART_Transmit(&huart4, (uint8_t*)"+++", 3, 1000);
    HAL_Delay(500);
    ESP8266_SendCmd("AT+CIPCLOSE\r\n", "OK", 1000);
    ESP8266_SendCmd("AT+CIPMODE=0\r\n", "OK", 500);
    ESP8266_SendCmd("AT+CIPMUX=0\r\n", "OK", 500);
    ESP8266_SendCmd("AT+CIPMODE=1\r\n", "OK", 500);

    sprintf(cmd_buf, "AT+CIPSTART=\"TCP\",\"%s\",80\r\n", SERVER_DOMAIN);
    if(ESP8266_SendCmd(cmd_buf, "OK", 5000) == 0) return;

    if(ESP8266_SendCmd("AT+CIPSEND\r\n", ">", 2000) == 0) return;

    // 转换 Base64
    uint32_t b64_len = Encode_Base64(img_ptr, img_size, b64_buffer);
    printf("转换后的 Base64 长度: %lu 字节\r\n", (unsigned long)b64_len);
    sprintf(http_header, 
            "POST %s HTTP/1.1\r\n"
            "Host: %s\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: %lu\r\n"
            "Connection: close\r\n\r\n", 
            HTTP_PATH, SERVER_DOMAIN, (unsigned long)b64_len);

    // 发送 Header
    HAL_UART_Transmit(&huart4, (uint8_t*)http_header, strlen(http_header), 1000);

    // 清洗 Cache (H7 必须对齐)
    uint32_t cache_clean_len = ((b64_len + 31) / 32) * 32;
    if(cache_clean_len > 30000) cache_clean_len = 30000;
    SCB_CleanDCache_by_Addr((uint32_t *)b64_buffer, cache_clean_len);
    
    uint32_t send_size = 0;
    uint32_t chunk_size = 1024; 
    
    for(uint32_t offset = 0; offset < b64_len; offset += chunk_size)
    {
        send_size = ((b64_len - offset) >= chunk_size) ? chunk_size : (b64_len - offset);
        
        uart_tx_complete = 0; 
        HAL_UART_Transmit_DMA(&huart4, (uint8_t *)(b64_buffer + offset), send_size);
        
        uint32_t dma_timeout = HAL_GetTick();
        while(uart_tx_complete == 0) {
            if((HAL_GetTick() - dma_timeout) > 2000) return;
        } 
        
        // --- 核心调整：大图发送增加延时 ---
        // 如果图片很大，建议从 15ms 增加到 30ms 或 50ms
        // 给 ESP8266 足够的时间把这 1KB 发送到 WiFi
        HAL_Delay(50); 
    }
    
    // 给 ESP8266 留出最后的数据清空时间
    printf("-> 数据推送完毕，等待模块清空缓存...\r\n");
    HAL_Delay(3000); 
	
	printf("5. 正在断开透传...\r\n");
    HAL_UART_Transmit(&huart4, (uint8_t*)"+++", 3, 1000);
    HAL_Delay(1500); // 必须等 1.5 秒
    
    ESP8266_SendCmd("AT+CIPMODE=0\r\n", "OK", 1000);
    printf("--- 上传流程结束 ---\r\n\r\n");
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if(huart->Instance == UART4)
    {
        uart_tx_complete = 1; // 标记发送完成
    }
}

uint32_t Encode_Base64(uint8_t *src, uint32_t src_len, char *out) 
{
    static const char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    uint32_t i = 0, j = 0;
    
    for (i = 0; i < src_len; ) {
        uint32_t octet_a = i < src_len ? src[i++] : 0;
        uint32_t octet_b = i < src_len ? src[i++] : 0;
        uint32_t octet_c = i < src_len ? src[i++] : 0;

        uint32_t triple = (octet_a << 16) | (octet_b << 8) | octet_c;

        out[j++] = b64[(triple >> 18) & 0x3F];
        out[j++] = b64[(triple >> 12) & 0x3F];
        out[j++] = b64[(triple >> 6) & 0x3F];
        out[j++] = b64[triple & 0x3F];
    }

    // 自动补齐等号 '='
    int pad = src_len % 3;
    if (pad == 1) {
        out[j - 1] = '=';
        out[j - 2] = '=';
    } else if (pad == 2) {
        out[j - 1] = '=';
    }
    out[j] = '\0';
    return j; // 返回转换后的字符串长度
}

uint32_t Pack_All_Camera_Data(void)
{
    // 1. 写入总帧头 (4 字节)
    test_jpeg_buffer[0] = 0x5A;
    test_jpeg_buffer[1] = 0x5A;
    test_jpeg_buffer[2] = 0x04; // 4路数据
    test_jpeg_buffer[3] = 0x06;

    // 2. 拷贝 4 路像素数据 (共 6144 字节)
    // 直接从每个 cam_buf 的第 4 字节开始拷贝 1536 字节的像素
    memcpy(&test_jpeg_buffer[4],            &cam1_buf[0], 1536);
    memcpy(&test_jpeg_buffer[4 + 1536],     &cam1_buf[0], 1536);
    memcpy(&test_jpeg_buffer[4 + 1536 * 2], &cam1_buf[0], 1536);
    memcpy(&test_jpeg_buffer[4 + 1536 * 3], &cam1_buf[0], 1536);

    // 3. 写入 AI 分析得分 (2 字节)
    // 位置：6148 和 6149
    uint32_t current_ptr = 4 + 6144; 
    test_jpeg_buffer[current_ptr++] = (int8_t)fall_score;   // 跌倒概率
    test_jpeg_buffer[current_ptr++] = (int8_t)normal_score; // 正常概率

    // 4. 写入雷达数据 (5 字节)
    // 位置：6150 - 6154
    memcpy(&test_jpeg_buffer[current_ptr], &g_radar_data.height, 4);
    current_ptr += 4;
    test_jpeg_buffer[current_ptr++] = g_radar_data.is_fall;

    // 5. 返回最终固定长度：6155
    return current_ptr; 
}
