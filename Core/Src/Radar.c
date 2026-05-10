#include "Radar.h"
#include "usart.h"
#include <string.h>
#include <stdio.h>


uint8_t radar_rx_buf[RADAR_BUF_SIZE];
volatile uint16_t radar_data_len = 0;
volatile uint8_t radar_ready_flag = 0;
RadarData_t g_radar_data = {0};

void Start_Radar_DMA(void) {
    // 开启空闲中断，DMA循环模式由 CubeMX 配置决定
    __HAL_UART_ENABLE_IT(&huart7, UART_IT_IDLE);
    HAL_UART_Receive_DMA(&huart7, radar_rx_buf, RADAR_BUF_SIZE);
}

void ParseRadarData_DMA(void) {
    if (radar_ready_flag == 1) {
        // 1. 确保读取到的是最新的物理内存数据
        SCB_InvalidateDCache_by_Addr((uint32_t *)radar_rx_buf, radar_data_len);

        // 2. 帧头判断 (假设帧头是 0x01，根据你的手册修改)
        if (radar_data_len >= 9 && radar_rx_buf[0] == 0x01) {
            
            // 提取数据长度
            uint16_t data_len = (radar_rx_buf[3] << 8) | radar_rx_buf[4];
            uint16_t type = (radar_rx_buf[5] << 8) | radar_rx_buf[6];
            
            // 3. 解析高度 (Type: 0x0E0E)
            if (type == 0x0E0E && data_len == 4) {
                memcpy(&g_radar_data.height, &radar_rx_buf[8], 4);
                g_radar_data.last_update_time = HAL_GetTick();
            }
            
            // 4. 解析跌倒 (Type: 0x0E02)
            else if (type == 0x0E02 && data_len == 1) {
                g_radar_data.is_fall = radar_rx_buf[8];
                g_radar_data.last_update_time = HAL_GetTick();
            }
        }
        
        radar_ready_flag = 0; // 处理完毕，清除标志
    }
}
