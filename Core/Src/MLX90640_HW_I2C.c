#include "MLX90640_HW_I2C.h"
#include "i2c.h"             

// 1. 声明外部的动态 I2C 指针（这个指针将在 main.c 中定义和切换）
//extern I2C_HandleTypeDef* active_hi2c; 

// =========================================================================
// I2C 初始化与复位 (保持不变)
// =========================================================================
void MLX90640_I2CInit(void) {}
int MLX90640_I2CGeneralReset(void) { return 0; }

// =========================================================================
// 硬件 I2C 极速读取
// =========================================================================
int MLX90640_I2CRead(uint8_t slaveAddr, uint16_t startAddress, uint16_t nMemAddressRead, uint16_t *data)
{
    uint8_t i2c_addr = slaveAddr << 1; 

    // 2. 将 &hi2c1 替换为 active_hi2c
    if(HAL_I2C_Mem_Read(&hi2c1, i2c_addr, startAddress, I2C_MEMADD_SIZE_16BIT, (uint8_t*)data, nMemAddressRead * 2, 500) == HAL_OK)
    {
        for(int i = 0; i < nMemAddressRead; i++)
        {
            uint16_t tmp = data[i];
            data[i] = (tmp >> 8) | (tmp << 8); 
        }
        return 0; 
    }
    return -1; 
}

// =========================================================================
// 硬件 I2C 极速写入
// =========================================================================
int MLX90640_I2CWrite(uint8_t slaveAddr, uint16_t writeAddress, uint16_t data)
{
    uint8_t i2c_addr = slaveAddr << 1;
    uint16_t data_swapped = (data >> 8) | (data << 8);

    // 3. 将 &hi2c1 替换为 active_hi2c
    if(HAL_I2C_Mem_Write(&hi2c1, i2c_addr, writeAddress, I2C_MEMADD_SIZE_16BIT, (uint8_t*)&data_swapped, 2, 500) == HAL_OK)
    {
        return 0; 
    }
    return -1; 
}
