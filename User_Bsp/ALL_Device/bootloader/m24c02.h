#ifndef M24C02_H
#define M24C02_H

#include "stdint.h"
#include "main.h"

#define M24C02_WADDR 0xA0 // 24C02写操作器件地址
#define M24C02_RADDR 0xA1 // 24C02读操作器件地址

#define IIC_SCL_H HAL_GPIO_WritePin(SCL_PB6_GPIO_Port, SCL_PB6_Pin, GPIO_PIN_SET)
#define IIC_SCL_L HAL_GPIO_WritePin(SCL_PB6_GPIO_Port, SCL_PB6_Pin, GPIO_PIN_RESET)
#define IIC_SDA_H HAL_GPIO_WritePin(SDA_PB7_GPIO_Port, SDA_PB7_Pin, GPIO_PIN_SET)
#define IIC_SDA_L HAL_GPIO_WritePin(SDA_PB7_GPIO_Port, SDA_PB7_Pin, GPIO_PIN_RESET)
#define READ_SDA HAL_GPIO_ReadPin(SDA_PB7_GPIO_Port, SDA_PB7_Pin)

#define BIT(x) ((uint32_t)((uint32_t)0x01U << (x)))
uint8_t M24C02_WriteByte(uint8_t addr, uint8_t wdata);                   // 函数声明
uint8_t M24C02_WritePage(uint8_t addr, uint8_t *wdata);                  // 函数声明
uint8_t M24C02_ReadData(uint8_t addr, uint8_t *rdata, uint16_t datalen); // 函数声明
void M24C02_ReadOTAInfo(void);                                           // 函数声明
void M24C02_WriteOTAInfo(void);                                          // 函数声明
void M24C02_Test(void);
#endif
