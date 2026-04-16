#include "MLX90640_HW_I2C.h" // ⚠️ 已经改成了你自定义的头文件名字
#include "i2c.h"             // 引入 CubeMX 生成的 I2C 头文件

// 声明外部的 I2C 句柄 (CubeMX 自动在 i2c.c 里生成的那个，如果接的是I2C2请改hi2c2)
extern I2C_HandleTypeDef hi2c1; 

// =========================================================================
// 1. I2C 初始化
// =========================================================================
void MLX90640_I2CInit(void)
{
    // 你的 main.c 里面已经由 CubeMX 调用过 MX_I2C1_Init() 了，所以这里留空
}

// =========================================================================
// 2. I2C 软件复位
// =========================================================================
int MLX90640_I2CGeneralReset(void)
{
    return 0; // 通常用不到，直接返回 0 表示成功
}

// =========================================================================
// 3. 硬件 I2C 极速读取 (核心！)
// =========================================================================
int MLX90640_I2CRead(uint8_t slaveAddr, uint16_t startAddress, uint16_t nMemAddressRead, uint16_t *data)
{
    // STM32 要求 I2C 地址必须左移 1 位
    uint8_t i2c_addr = slaveAddr << 1; 

    // 使用硬件 I2C 极速读取
    if(HAL_I2C_Mem_Read(&hi2c1, i2c_addr, startAddress, I2C_MEMADD_SIZE_16BIT, (uint8_t*)data, nMemAddressRead * 2, 500) == HAL_OK)
    {
        // ⚠️ 大小端翻转 (Endianness Swap)
        // 传感器是大端，STM32是小端，必须翻转，否则温度算出来错乱！
        for(int i = 0; i < nMemAddressRead; i++)
        {
            uint16_t tmp = data[i];
            data[i] = (tmp >> 8) | (tmp << 8); 
        }
        return 0; // 成功
    }
    
    return -1; // 失败
}

// =========================================================================
// 4. 硬件 I2C 极速写入 (核心！)
// =========================================================================
int MLX90640_I2CWrite(uint8_t slaveAddr, uint16_t writeAddress, uint16_t data)
{
    uint8_t i2c_addr = slaveAddr << 1;
    
    // 写入前，翻转成传感器认识的大端格式
    uint16_t data_swapped = (data >> 8) | (data << 8);

    // 发送指令
    if(HAL_I2C_Mem_Write(&hi2c1, i2c_addr, writeAddress, I2C_MEMADD_SIZE_16BIT, (uint8_t*)&data_swapped, 2, 500) == HAL_OK)
    {
        return 0; // 成功
    }
    
    return -1; // 失败
}
