/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "spi.h"
#include "usart.h"
#include "gpio.h"
#include "lcd.h"
#include "lcd_init.h"
#include "pic.h"
#include <stdio.h>
#include "MLX90640_API.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#define MLX90640_ADDR 0x33 // 传感器 I2C 地址

// 内存大户全部丢进 RAM_D1 高速区
__attribute__((section(".RAM_D1"), aligned(32))) uint16_t eeMLX90640[832];
__attribute__((section(".RAM_D1"), aligned(32))) uint16_t frameData[834];
__attribute__((section(".RAM_D1"), aligned(32))) uint16_t raw_temp_int[768]; 
uint8_t pc_tx_buf[1544] = {0}; // 伪装给上位机发的 1544 字节包
__attribute__((section(".RAM_D1"), aligned(32))) uint16_t display_data_array[12288]; 
float raw_temperature[768]; 
paramsMLX90640 mlx90640; 
// 光学参数结构体

int fputc(int ch, FILE *f)
{
    HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}
// 辅助函数：防止数组越界
float Get_Temp_Safe(float* raw_temp, int x, int y) {
    if (x < 0) x = 0;
    if (x >= 32) x = 31;
    if (y < 0) y = 0;
    if (y >= 24) y = 23;
    return raw_temp[y * 32 + x];
}

void Preprocess_Thermal_Colormapped(float* raw_temp, uint16_t* display_rgb565) 
{
    // ==========================================
    // 1. 计算真实的画面平均温度（通常代表背景）
    // ==========================================
    float sum_t = 0;
    for(int i = 0; i < 768; i++) {
        sum_t += raw_temp[i];
    }
    float avg_t = sum_t / 768.0f;

    // 强行拉开色差：比平均温度低1.5度的就是背景(蓝绿)，比平均高4度的就是人体(红白)
    float min_temp = avg_t - 1.5f; 
    float max_temp = avg_t + 4.0f;

    // ==========================================
    // 2. 直接生成 128x96 的高清平滑图像（放大4倍）
    // ==========================================
    for(int y = 0; y < 96; y++) {
        for(int x = 0; x < 128; x++) {
            
            // 算出在 32x24 原始数组里的浮点坐标
            float gx = x / 4.0f; 
            float gy = y / 4.0f;
            
            int gxi = (int)gx;
            int gyi = (int)gy;
            
            float tx = gx - gxi;
            float ty = gy - gyi;
            
            // 取出四个点做双线性插值
            float c00 = Get_Temp_Safe(raw_temp, gxi, gyi);
            float c10 = Get_Temp_Safe(raw_temp, gxi + 1, gyi);
            float c01 = Get_Temp_Safe(raw_temp, gxi, gyi + 1);
            float c11 = Get_Temp_Safe(raw_temp, gxi + 1, gyi + 1);
            
            float temp = (1 - tx) * (1 - ty) * c00 +
                         tx * (1 - ty) * c10 +
                         (1 - tx) * ty * c01 +
                         tx * ty * c11;

            // 铁红伪彩色映射
            float v = (temp - min_temp) / (max_temp - min_temp);
            if(v > 1.0f) v = 1.0f; 
            if(v < 0.0f) v = 0.0f;

            float r_f, g_f, b_f;
            if (v < 0.125f) {
                r_f = 0.0f; g_f = 0.0f; b_f = 0.5f + 4.0f * v;
            } else if (v < 0.375f) {
                r_f = 0.0f; g_f = 4.0f * (v - 0.125f); b_f = 1.0f;
            } else if (v < 0.625f) {
                r_f = 4.0f * (v - 0.375f); g_f = 1.0f; b_f = 1.0f - 4.0f * (v - 0.375f);
            } else if (v < 0.875f) {
                r_f = 1.0f; g_f = 1.0f - 4.0f * (v - 0.625f); b_f = 0.0f;
            } else {
                r_f = 1.0f - 4.0f * (v - 0.875f); g_f = 0.0f; b_f = 0.0f;
            }

            uint16_t r = (uint16_t)(r_f * 31.0f);
            uint16_t g = (uint16_t)(g_f * 63.0f);
            uint16_t b = (uint16_t)(b_f * 31.0f);
            
            // 存入一维数组，现在的跨度是 128
            display_rgb565[y * 128 + x] = (r << 11) | (g << 5) | b;
        }
    }
}
// =========================================================================
// 注意：移除了 scale 参数
void LCD_ShowThermal_Map(uint16_t start_x, uint16_t start_y, uint16_t* rgb565_data)
{
    uint16_t disp_w = 128; 
    uint16_t disp_h = 96; 
    
    // 圈地运动：128x96 的长方形
    LCD_Address_Set(start_x, start_y, start_x + disp_w - 1, start_y + disp_h - 1);
    
    // 一次性把 12288 个像素极速发完
    for(uint32_t i = 0; i < disp_w * disp_h; i++) 
    {
        uint16_t color565 = rgb565_data[i];
        LCD_WR_DATA8(color565 >> 8);   
        LCD_WR_DATA8(color565 & 0xFF); 
    }
}
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* Enable the CPU Cache */

  /* Enable I-Cache---------------------------------------------------------*/
  SCB_EnableICache();

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_SPI1_Init();
  MX_I2C1_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  LCD_Init();//LCD
  LCD_Fill(0,0,LCD_W,LCD_H,WHITE);
	LCD_ShowString(10, 10, (uint8_t*)"System Boot...", RED, BLACK, 24, 0);
  
  printf("MLX90640 Initialization Starting...\r\n");
  
  // 设置刷新率 (0x04代表8Hz, 0x05代表16Hz)
  MLX90640_SetRefreshRate(MLX90640_ADDR, 0x04); 
  MLX90640_SetChessMode(MLX90640_ADDR); 
// 设置交错模式

  // 提取出厂秘方
  if(MLX90640_DumpEE(MLX90640_ADDR, eeMLX90640) != 0) 
  {
      printf("I2C Error: Cannot read EEPROM! Check wiring and pull-up resistors.\r\n");
      while(1); // 如果报错，会死等在这里
  }
  
  MLX90640_ExtractParameters(eeMLX90640, &mlx90640);
  printf("MLX90640 Init Success! Starting data stream...\r\n");
	  LCD_ShowString(10, 40, (uint8_t*)"Init Success!", GREEN, BLACK, 24, 0);
  HAL_Delay(500); 
  LCD_Fill(0, 0, LCD_W, LCD_H, BLACK); // 清屏，准备迎接热像图
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
//	  LCD_ShowPicture(0,0,230,230,gImage_5);
            
      if(MLX90640_GetFrameData(MLX90640_ADDR, frameData) >= 0)
      {
          float Ta = MLX90640_GetTa(frameData, &mlx90640);
          
          // 算出 768 个像素的整数温度
          MLX90640_CalculateTo(frameData, &mlx90640, 0.95f, Ta - 8.0f, raw_temp_int);
				            if (frameData[833] == 1) 
          {
              // 4. 将整数还原成真实浮点温度
              for(int i = 0; i < 768; i++) 
              {
                  raw_temperature[i] = (float)raw_temp_int[i] / 100.0f;
              }

              // 5. 映射颜色并极速刷屏
              Preprocess_Thermal_Colormapped(raw_temperature, display_data_array);
              LCD_ShowThermal_Map(56, 72, display_data_array);

              // 6. 显示温度文字
              char temp_str[30];
              sprintf(temp_str, "C:%.1f  T:%.1f", raw_temperature[384], Ta);
              LCD_ShowString(10, 2, (uint8_t*)temp_str, WHITE, BLACK, 16, 0);
          }
      }
	  
	  
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 10;
  RCC_OscInitStruct.PLL.PLLN = 220;
  RCC_OscInitStruct.PLL.PLLP = 1;
  RCC_OscInitStruct.PLL.PLLQ = 5;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_1;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
