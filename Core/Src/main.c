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
#include "dma.h"
#include "i2c.h"
#include "spi.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "lcd.h"
#include "lcd_init.h"
#include <stdio.h>
#include "MLX90640_API.h"
#include <string.h>
#include "network.h"       // AI 网络主头文件
#include "network_data.h"  // AI 权重数据头文�?
#include <stdio.h>
#include <math.h>
#include "LCD_show.h"
#include "ESP8266.h"
#include "Radar.h"
#define MLX90640_ADDR 0x33 // 传感�? I2C 地址
#if defined(__ICCARM__) || defined(__CC_ARM) || defined(__GNUC__)
    __attribute__((aligned(32))) char b64_buffer[30000]; 
#endif

#if defined(__ICCARM__) || defined(__CC_ARM) || defined(__GNUC__)
    __attribute__((aligned(32))) uint8_t test_jpeg_buffer[7500];
#endif

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
int8_t fall_score,normal_score;
__attribute__((section(".RAM_D1"), aligned(32))) ai_u8 activations[AI_NETWORK_DATA_ACTIVATIONS_SIZE];
__attribute__((section(".RAM_D1"), aligned(32))) ai_i8 ai_in_data[AI_NETWORK_IN_1_SIZE_BYTES];
__attribute__((section(".RAM_D1"), aligned(32))) ai_i8 ai_out_data[AI_NETWORK_OUT_1_SIZE_BYTES];
char tx_buffer[128]; 
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

    int8_t fall_score = out_ptr[0];
    int8_t normal_score = out_ptr[1];
		snprintf(tx_buffer, sizeof(tx_buffer), ">> AI CHECK: Fall(%d) vs Normal(%d)\n", fall_score, normal_score);
		HAL_UART_Transmit(&huart2, (uint8_t *)tx_buffer, strlen(tx_buffer), HAL_MAX_DELAY);
		snprintf(tx_buffer, sizeof(tx_buffer), "AI Executed -> Fall:%d Normal:%d\r\n", fall_score, normal_score);
		HAL_UART_Transmit(&huart2, (uint8_t *)tx_buffer, strlen(tx_buffer), HAL_MAX_DELAY);
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
static void MPU_Config(void);
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

  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

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
  MX_DMA_Init();
  MX_I2C1_Init();
  MX_USART2_UART_Init();
  MX_SPI1_Init();
  MX_USART3_UART_Init();
  MX_UART4_Init();
  MX_UART7_Init();
  /* USER CODE BEGIN 2 */
//  LCD_Init();//LCD
//  LCD_Fill(0,0,LCD_W,LCD_H,WHITE);
	//Internet port
	Start_Radar_DMA(); 
	ESP8266_Init_WiFi();
	HAL_Delay(3000);
  printf("--- Waiting Test ---\r\n");
	HAL_UART_Transmit(&huart2, (uint8_t *)"MLX90640 Initialization Starting...\r\n", sizeof("MLX90640 Initialization Starting...\r\n")-1, 100);
  memset(display_data_array, 0, sizeof(display_data_array));
  MLX90640_SetRefreshRate(MLX90640_ADDR, 0x05); 
  MLX90640_SetChessMode(MLX90640_ADDR); 

  if(MLX90640_DumpEE(MLX90640_ADDR,cam.eeMLX90640) != 0) {
          HAL_UART_Transmit(&huart2, (uint8_t *)"error\r\n", sizeof("error\r\n")-1, 100);
      } else {
          MLX90640_ExtractParameters(cam.eeMLX90640, &cam.mlx_params);
				HAL_UART_Transmit(&huart2, (uint8_t *)"Cam Init Success!\r\n", sizeof("Cam Init Success!\r\n")-1, 100);
      }
		  
// ==================== AI 引擎初始�? ====================
	HAL_UART_Transmit(&huart2, (uint8_t *)"AI Engine Starting...\r\n", sizeof("AI Engine Starting...\r\n")-1, 100);
  ai_error err = ai_network_create(&network_handle, AI_NETWORK_DATA_CONFIG);
  if (err.type != AI_ERROR_NONE) {
		HAL_UART_Transmit(&huart2, (uint8_t *)"AI Create Failed!\r\n", sizeof("AI Create Failed!\r\n")-1, 100);
  } else {
      const ai_network_params params = {
          AI_NETWORK_DATA_WEIGHTS(ai_network_data_weights_get()),
          AI_NETWORK_DATA_ACTIVATIONS(activations)
      };
      if (!ai_network_init(network_handle, &params)) {
				HAL_UART_Transmit(&huart2, (uint8_t *)"AI Init Failed!\r\n", sizeof("AI Init Failed!\r\n")-1, 100);
      } else {
          ai_input = ai_network_inputs_get(network_handle, NULL);
          ai_output = ai_network_outputs_get(network_handle, NULL);
				HAL_UART_Transmit(&huart2, (uint8_t *)"AI Engine Ready! Let's Go!\r\n", sizeof("AI Engine Ready! Let's Go!\r\n")-1, 100);
      }
  }
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
									Send_IR_Data_Hex(cam.raw_temp_int); 

									 // 5. 召唤 AI 进行跌�?�判定！
                    if (HAL_GetTick() - last_ai_time >= 5000) 
                      {
                        Run_Fall_Detection_AI(cam.raw_temperature);
                        last_ai_time = HAL_GetTick(); // 重置计时器
                      }
									 ParseRadarData_DMA();

                   // 2. 检查是否有新的数据包需要上传 (比如通过定时器或雷达触发)
                   // 示例：每 10 秒上传一次，或者检测到跌倒 (g_radar_data.is_fall == 1) 时触发
                   static uint32_t last_upload_time = 0;
                   if ((HAL_GetTick() - last_upload_time > 10000) || ((g_radar_data.is_fall == 1)&&(fall_score > normal_score && fall_score > 50)))
                      {
                          last_upload_time = HAL_GetTick();
        
                          printf("-- Intend Transmit --- \r\n");
                          uint32_t total_size = Pack_All_Camera_Data();
        
                          printf("All_Length: %lu byte\r\n", (unsigned long)total_size);
		                          // 打包完数据后，确保内存已刷新
                          SCB_CleanDCache_by_Addr((uint32_t *)test_jpeg_buffer, 7500);
                          Upload_Image_To_Cloud(test_jpeg_buffer, total_size);												
                      }
                    HAL_Delay (10);
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

 /* MPU Configuration */

void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x0;
  MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
  MPU_InitStruct.SubRegionDisable = 0x87;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

}

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
