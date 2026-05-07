#include "LCD_show.h"

float Get_Temp_Safe(float* raw_temp, int x, int y) {
    if (x < 0) x = 0;
    if (x >= 32) x = 31;
    if (y < 0) y = 0;
    if (y >= 24) y = 23;
    return raw_temp[y * 32 + x];
}


void Preprocess_Thermal_Colormapped(float* raw_temp, uint16_t* display_rgb565) 
{
    float sum_t = 0;
    for(int i = 0; i < 768; i++) sum_t += raw_temp[i];
    float avg_t = sum_t / 768.0f;


    float min_temp = avg_t - 2.0f; 
    float max_temp = avg_t + 5.0f;

    for(int y = 0; y < 96; y++) {
        for(int x = 0; x < 128; x++) {
            float gx = x / 4.0f; float gy = y / 4.0f;
            int gxi = (int)gx; int gyi = (int)gy;
            float tx = gx - gxi; float ty = gy - gyi;
            
            float c00 = Get_Temp_Safe(raw_temp, gxi, gyi);
            float c10 = Get_Temp_Safe(raw_temp, gxi + 1, gyi);
            float c01 = Get_Temp_Safe(raw_temp, gxi, gyi + 1);
            float c11 = Get_Temp_Safe(raw_temp, gxi + 1, gyi + 1);
            
            float temp = (1 - tx) * (1 - ty) * c00 + tx * (1 - ty) * c10 + (1 - tx) * ty * c01 + tx * ty * c11;

            float v = (temp - min_temp) / (max_temp - min_temp);
            if(v > 1.0f) v = 1.0f; if(v < 0.0f) v = 0.0f;

            float r_f, g_f, b_f;
            if (v < 0.125f) { r_f = 0.0f; g_f = 0.0f; b_f = 0.5f + 4.0f * v; }
            else if (v < 0.375f) { r_f = 0.0f; g_f = 4.0f * (v - 0.125f); b_f = 1.0f; }
            else if (v < 0.625f) { r_f = 4.0f * (v - 0.375f); g_f = 1.0f; b_f = 1.0f - 4.0f * (v - 0.375f); }
            else if (v < 0.875f) { r_f = 1.0f; g_f = 1.0f - 4.0f * (v - 0.625f); b_f = 0.0f; }
            else { r_f = 1.0f - 4.0f * (v - 0.875f); g_f = 0.0f; b_f = 0.0f; }

            uint16_t r = (uint16_t)(r_f * 31.0f);
            uint16_t g = (uint16_t)(g_f * 63.0f);
            uint16_t b = (uint16_t)(b_f * 31.0f);
            display_rgb565[y * 128 + x] = (r << 11) | (g << 5) | b;
        }
    }
}
// =========================================================================

void LCD_ShowThermal_Map(uint16_t start_x, uint16_t start_y, uint16_t* rgb565_data)
{
    uint16_t disp_w = 128; 
    uint16_t disp_h = 96; 
    
    // ????_128x96 ????
    LCD_Address_Set(start_x, start_y, start_x + disp_w - 1, start_y + disp_h - 1);
    

    for(uint32_t i = 0; i < disp_w * disp_h; i++) 
    {
        uint16_t color565 = rgb565_data[i];
        LCD_WR_DATA8(color565 >> 8);   
        LCD_WR_DATA8(color565 & 0xFF); 
    }
}
