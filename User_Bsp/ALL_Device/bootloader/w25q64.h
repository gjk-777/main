#ifndef W25Q64_H
#define W25Q64_H

#include "stdint.h"
#include "main.h"
#define CS_ENABLE HAL_GPIO_WritePin(w25q64_cs_GPIO_Port, w25q64_cs_Pin, GPIO_PIN_RESET)
#define CS_DISENABLE HAL_GPIO_WritePin(w25q64_cs_GPIO_Port, w25q64_cs_Pin, GPIO_PIN_SET)

void W25Q64_WaitBusy(void);                                        // 函数声明
void W25Q64_Enable(void);                                          // 函数声明
void W25Q64_Erase64K(uint8_t blockNB);                             // 函数声明
void W25Q64_PageWrite(uint8_t *wbuff, uint16_t pageNB);            // 函数声明
void W25Q64_Read(uint8_t *rbuff, uint32_t addr, uint32_t datalen); // 函数声明
void w25q64_test(void);
void stm32_Erase_System_flash(uint32_t start_addr, uint16_t num);             // 函数声明：擦除FLASH
void stm32_System_WriteFlash(uint32_t saddr, uint32_t *wdata, uint32_t wnum); // 函数声明：写入FLASH
#endif
