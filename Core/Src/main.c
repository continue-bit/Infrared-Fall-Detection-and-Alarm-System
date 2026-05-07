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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "lcd.h"
#include "lcd_init.h"
#include "pic.h"
#include <stdio.h>
#include "MLX90640_API.h"
#include <string.h>
#include "network.h"       // AI 网络主头文件
#include "network_data.h"  // AI 权重数据头文�?
#include <stdio.h>
#include <math.h>
#include "LCD_show.h"
#define MLX90640_ADDR 0x33 // 传感�? I2C 地址
typedef struct {
    uint16_t eeMLX90640[832];
    uint16_t frameData[834];
    uint16_t raw_temp_int[768]; 
    float raw_temperature[768]; 
    paramsMLX90640 mlx_params;
} MLX90640_Camera;
__attribute__((section(".RAM_D1"), aligned(32))) MLX90640_Camera cam;
__attribute__((section(".RAM_D1"), aligned(32))) uint16_t display_data_array[12288];
//AI变量
ai_handle network_handle = AI_HANDLE_NULL;
ai_buffer *ai_input;
ai_buffer *ai_output;
__attribute__((section(".RAM_D1"), aligned(32))) ai_u8 activations[AI_NETWORK_DATA_ACTIVATIONS_SIZE];
__attribute__((section(".RAM_D1"), aligned(32))) ai_i8 ai_in_data[AI_NETWORK_IN_1_SIZE_BYTES];
__attribute__((section(".RAM_D1"), aligned(32))) ai_i8 ai_out_data[AI_NETWORK_OUT_1_SIZE_BYTES];
extern UART_HandleTypeDef huart2; 
int fputc(int ch, FILE *f)
{
    HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}
//float Get_Temp_Safe(float* raw_temp, int x, int y) {
//    if (x < 0) x = 0;
//    if (x >= 32) x = 31;
//    if (y < 0) y = 0;
//    if (y >= 24) y = 23;
//    return raw_temp[y * 32 + x];
//}

//// 1. 优化后的热成像图显示：避开文字区域，增加对比度
//void Preprocess_Thermal_Colormapped(float* raw_temp, uint16_t* display_rgb565) 
//{
//    float sum_t = 0;
//    for(int i = 0; i < 768; i++) sum_t += raw_temp[i];
//    float avg_t = sum_t / 768.0f;

//    // 调整显示对比度，让人体更鲜艳
//    float min_temp = avg_t - 2.0f; 
//    float max_temp = avg_t + 5.0f;

//    for(int y = 0; y < 96; y++) {
//        for(int x = 0; x < 128; x++) {
//            float gx = x / 4.0f; float gy = y / 4.0f;
//            int gxi = (int)gx; int gyi = (int)gy;
//            float tx = gx - gxi; float ty = gy - gyi;
//            
//            float c00 = Get_Temp_Safe(raw_temp, gxi, gyi);
//            float c10 = Get_Temp_Safe(raw_temp, gxi + 1, gyi);
//            float c01 = Get_Temp_Safe(raw_temp, gxi, gyi + 1);
//            float c11 = Get_Temp_Safe(raw_temp, gxi + 1, gyi + 1);
//            
//            float temp = (1 - tx) * (1 - ty) * c00 + tx * (1 - ty) * c10 + (1 - tx) * ty * c01 + tx * ty * c11;

//            float v = (temp - min_temp) / (max_temp - min_temp);
//            if(v > 1.0f) v = 1.0f; if(v < 0.0f) v = 0.0f;

//            float r_f, g_f, b_f;
//            if (v < 0.125f) { r_f = 0.0f; g_f = 0.0f; b_f = 0.5f + 4.0f * v; }
//            else if (v < 0.375f) { r_f = 0.0f; g_f = 4.0f * (v - 0.125f); b_f = 1.0f; }
//            else if (v < 0.625f) { r_f = 4.0f * (v - 0.375f); g_f = 1.0f; b_f = 1.0f - 4.0f * (v - 0.375f); }
//            else if (v < 0.875f) { r_f = 1.0f; g_f = 1.0f - 4.0f * (v - 0.625f); b_f = 0.0f; }
//            else { r_f = 1.0f - 4.0f * (v - 0.875f); g_f = 0.0f; b_f = 0.0f; }

//            uint16_t r = (uint16_t)(r_f * 31.0f);
//            uint16_t g = (uint16_t)(g_f * 63.0f);
//            uint16_t b = (uint16_t)(b_f * 31.0f);
//            display_rgb565[y * 128 + x] = (r << 11) | (g << 5) | b;
//        }
//    }
//}
//// =========================================================================
//// 注意：移除了 scale 参数
//void LCD_ShowThermal_Map(uint16_t start_x, uint16_t start_y, uint16_t* rgb565_data)
//{
//    uint16_t disp_w = 128; 
//    uint16_t disp_h = 96; 
//    
//    // 圈地运动�?128x96 的长方形
//    LCD_Address_Set(start_x, start_y, start_x + disp_w - 1, start_y + disp_h - 1);
//    
//    // �?次�?�把 12288 个像素极速发�?
//    for(uint32_t i = 0; i < disp_w * disp_h; i++) 
//    {
//        uint16_t color565 = rgb565_data[i];
//        LCD_WR_DATA8(color565 >> 8);   
//        LCD_WR_DATA8(color565 & 0xFF); 
//    }
//}
uint32_t last_ai_time = 0;

void Run_Fall_Detection_AI(float* raw_temp) 
{
    if(network_handle == AI_HANDLE_NULL) return;

    int8_t* in_ptr = (int8_t*)ai_input[0].data;
    int8_t* out_ptr = (int8_t*)ai_output[0].data;

    float scale = 0.00670511368f; 
    int zero_point = 16;
    float min_t = 20.0f; 
    float max_t = 35.0f;

    // 图像处理逻辑 (32x24 -> 32x32)
    for(int y = 0; y < 32; y++) {
        int src_y = (y * 23) / 31; 
        for(int x = 0; x < 32; x++) {
            float temp = raw_temp[src_y * 32 + x]; 
            if(temp > max_t) temp = max_t;
            if(temp < min_t) temp = min_t;
            float v = (temp - min_t) / (max_t - min_t);
            float pixel_val = v * 255.0f;
            float float_val = (pixel_val / 127.5f) - 1.0f;
            int32_t int8_val = (int32_t)roundf(float_val / scale) + zero_point;
            in_ptr[y * 32 + x] = (int8_t)__SSAT(int8_val, 8);
        }
    }

    // 运行 AI
    SCB_CleanDCache_by_Addr((uint32_t*)ai_in_data, AI_NETWORK_IN_1_SIZE_BYTES);
    ai_network_run(network_handle, ai_input, ai_output);
    SCB_InvalidateDCache_by_Addr((uint32_t*)ai_out_data, AI_NETWORK_OUT_1_SIZE_BYTES);

    // --- LCD 结果强制刷新 ---
    // 1. 先把文字区域刷黑 (坐标 180 以下)
    LCD_Fill(0, 180, 240, 240, BLACK); 

    int8_t fall_score = out_ptr[0];
    int8_t normal_score = out_ptr[1];
    printf(">> AI CHECK: Fall(%d) vs Normal(%d)\n", fall_score, normal_score);
		
    if (fall_score > normal_score) {
        // 红色显示跌倒
        LCD_ShowString(10, 10, (uint8_t*)"FALL DETECTED!", WHITE, RED, 24, 0);
    } else
 {
        // 绿色显示正常
        LCD_ShowString(10, 10, (uint8_t*)"NORMAL", GREEN, BLACK, 24, 0);
    }
    
    printf("AI Executed -> Fall:%d Normal:%d\r\n", fall_score, normal_score);
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

  /* Enable D-Cache---------------------------------------------------------*/
  SCB_EnableDCache();

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
  MX_I2C1_Init();
  MX_USART2_UART_Init();
  MX_SPI1_Init();
  /* USER CODE BEGIN 2 */
  LCD_Init();//LCD
  LCD_Fill(0,0,LCD_W,LCD_H,WHITE);
//	LCD_ShowString(10, 10, (uint8_t*)"System Boot...", RED, BLACK, 24, 0);
  
  printf("MLX90640 Initialization Starting...\r\n");
	  
  memset(display_data_array, 0, sizeof(display_data_array));
  MLX90640_SetRefreshRate(MLX90640_ADDR, 0x05); 
  MLX90640_SetChessMode(MLX90640_ADDR); 

  if(MLX90640_DumpEE(MLX90640_ADDR,cam.eeMLX90640) != 0) {
          printf("Cam %d I2C Error! Check wiring.\r\n", 1);
      } else {
          MLX90640_ExtractParameters(cam.eeMLX90640, &cam.mlx_params);
          printf("Cam %d Init Success!\r\n",1);
      }
		  
// ==================== AI 引擎初始�? ====================
  printf("AI Engine Starting...\r\n");
  ai_error err = ai_network_create(&network_handle, AI_NETWORK_DATA_CONFIG);
  if (err.type != AI_ERROR_NONE) {
      printf("AI Create Failed!\r\n");
  } else {
      const ai_network_params params = {
          AI_NETWORK_DATA_WEIGHTS(ai_network_data_weights_get()),
          AI_NETWORK_DATA_ACTIVATIONS(activations)
      };
      if (!ai_network_init(network_handle, &params)) {
          printf("AI Init Failed!\r\n");
      } else {
          ai_input = ai_network_inputs_get(network_handle, NULL);
          ai_output = ai_network_outputs_get(network_handle, NULL);
          printf("AI Engine Ready! Let's Go!\r\n");
      }
  }
  LCD_Fill(0, 0, LCD_W, LCD_H, BLACK); 
	last_ai_time = HAL_GetTick();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
     /* Infinite loop */
  /* USER CODE BEGIN WHILE */
          if(MLX90640_GetFrameData(MLX90640_ADDR, cam.frameData) >= 0)
          {
              float Ta = MLX90640_GetTa(cam.frameData, &cam.mlx_params);
              
              MLX90640_CalculateTo(cam.frameData, &cam.mlx_params, 0.95f, Ta - 8.0f, cam.raw_temp_int);

              if (cam.frameData[833] == 1) 
              {
                  // 将整数还原成真实浮点温度
                  for(int j = 0; j < 768; j++) 
                  {
                      cam.raw_temperature[j] = (float)cam.raw_temp_int[j] / 100.0f;
                  }
                      Preprocess_Thermal_Colormapped(cam.raw_temperature, display_data_array);
                      LCD_ShowThermal_Map(56, 72, display_data_array);

                      char temp_str[30];
                      sprintf(temp_str, "C1:%.1f  T:%.1f", cam.raw_temperature[384], Ta);
                      LCD_ShowString(10, 2, (uint8_t*)temp_str, WHITE, BLACK, 16, 0);
									              // 5. 召唤 AI 进行跌�?�判定！
                       if (HAL_GetTick() - last_ai_time >= 5000) 
                      {
                        Run_Fall_Detection_AI(cam.raw_temperature);
                        last_ai_time = HAL_GetTick(); // 重置计时器
                      }
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
