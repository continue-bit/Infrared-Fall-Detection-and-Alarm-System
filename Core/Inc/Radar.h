#ifndef __RADAR_H
#define __RADAR_H

#include "main.h"
#define RADAR_BUF_SIZE 1024

// 雷达状态结构体
typedef struct {
    float height;
    uint8_t is_fall;
    uint32_t last_update_time;
} RadarData_t;

extern RadarData_t g_radar_data;
extern volatile uint8_t radar_ready_flag;
extern uint8_t radar_rx_buf[];
extern volatile uint16_t radar_data_len;

// 函数声明
void Start_Radar_DMA(void);
void ParseRadarData_DMA(void);

#endif
