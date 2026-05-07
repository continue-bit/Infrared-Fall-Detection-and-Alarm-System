#ifndef _LCD_SHOW_H_
#define _LCD_SHOW_H_
#include <stdint.h>
#include "lcd.h"
#include "lcd_init.h"
float Get_Temp_Safe(float* raw_temp, int x, int y);
void Preprocess_Thermal_Colormapped(float* raw_temp, uint16_t* display_rgb565);
void LCD_ShowThermal_Map(uint16_t start_x, uint16_t start_y, uint16_t* rgb565_data);

#endif
