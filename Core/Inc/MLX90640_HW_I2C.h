#ifndef _MLX90640_HW_I2C_H_
#define _MLX90640_HW_I2C_H_

#include <stdint.h>

/* 官方 API 需要调用的底层硬件接口函数声明 */
void MLX90640_I2CInit(void);
int  MLX90640_I2CGeneralReset(void);
int  MLX90640_I2CRead(uint8_t slaveAddr, uint16_t startAddress, uint16_t nMemAddressRead, uint16_t *data);
int  MLX90640_I2CWrite(uint8_t slaveAddr, uint16_t writeAddress, uint16_t data);

#endif