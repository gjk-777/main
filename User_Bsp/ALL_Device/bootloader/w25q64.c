
#include "w25q64.h"
#include "stm32f1xx_hal.h"
//#include "stm32f1xx_hal_flash_ex.h"
#include "spi.h"
#include "stdio.h"
#include "stdlib.h"
//#include "stm32_hal_legacy.h"
/*-------------------------------------------------*/
/*函数名：等待W25Q64空闲                           */
/*参  数：无                                       */
/*返回值：无                                       */
/*-------------------------------------------------*/
void W25Q64_WaitBusy(void)
{
	uint8_t res; // 保存返回值

	do
	{									// 循环等待
		CS_ENABLE;						// 打开CS
		SPI0_ReadWriteByte(0x05);		// 发送命令0x05
		res = SPI0_ReadWriteByte(0xff); // 发送一个字节，从而才能读取一个字节，也就是W25Q64状态寄存器1的数据
		CS_DISENABLE;					// 关闭CS
	} while ((res & 0x01) == 0x01); // 等到BIT0成0，退出while循环，说明W25Q64空闲了
}
/*-------------------------------------------------*/
/*函数名：W25Q64写使能                             */
/*参  数：无                                       */
/*返回值：无                                       */
/*-------------------------------------------------*/
void W25Q64_Enable(void)
{
	W25Q64_WaitBusy();		  // 等待W25q64空闲
	CS_ENABLE;				  // 打开CS
	SPI0_ReadWriteByte(0x06); // 发送命令0x06
	CS_DISENABLE;			  // 关闭CS
}
/*-------------------------------------------------*/
/*函数名：W25Q64擦除一个64K大小的块                */
/*参  数：blockNB：块编号从0开始                   */
/*返回值：无                                       */
/*-------------------------------------------------*/
void W25Q64_Erase64K(uint8_t blockNB)
{
	uint8_t wdata[4]; // 保存命令和地址

	wdata[0] = 0xD8;						// 擦除指令0xD8
	wdata[1] = (blockNB * 64 * 1024) >> 16; // 需要擦除的块的地址
	wdata[2] = (blockNB * 64 * 1024) >> 8;	// 需要擦除的块的地址
	wdata[3] = (blockNB * 64 * 1024) >> 0;	// 需要擦除的块的地址

	W25Q64_WaitBusy();	  // 等待W25Q64空闲
	W25Q64_Enable();	  // 使能
	CS_ENABLE;			  // 打开CS
	SPI0_Write(wdata, 4); // 把wdata发送给W25Q64
	CS_DISENABLE;		  // 关闭CS
	W25Q64_WaitBusy();	  // 等待W25Q64空闲吗，确保擦除完毕后再退出W25Q64_Erase64K子函数
}
/*--------------------------------------------------------*/
/*函数名：W25Q64写入一页数据（256字节）                   */
/*参  数：wbuff：写入数据指针  pageNB：页编号从0开始      */
/*返回值：无                                              */
/*--------------------------------------------------------*/
void W25Q64_PageWrite(uint8_t *wbuff, uint16_t pageNB)
{
	uint8_t wdata[4]; // 保存命令和地址

	wdata[0] = 0x02;				 // 页写入数据指令0x02
	wdata[1] = (pageNB * 256) >> 16; // 需要写入数据的页地址
	wdata[2] = (pageNB * 256) >> 8;	 // 需要写入数据的页地址
	wdata[3] = (pageNB * 256) >> 0;	 // 需要写入数据的页地址

	W25Q64_WaitBusy();		// 等待W25Q64空闲
	W25Q64_Enable();		// 使能
	CS_ENABLE;				// 打开CS
	SPI0_Write(wdata, 4);	// 把wdata发送给W25Q64
	SPI0_Write(wbuff, 256); // 接着发送256字节（一页）的数据
	CS_DISENABLE;			// 关闭CS
}
/*-----------------------------------------------------------------*/
/*函数名：W25Q64读取数据                                           */
/*参  数：rbuff：接收缓冲区  addr：读取地址  datalen：读取长度     */
/*返回值：无                                                       */
/*-----------------------------------------------------------------*/
void W25Q64_Read(uint8_t *rbuff, uint32_t addr, uint32_t datalen)
{
	uint8_t wdata[4]; // 保存命令和地址

	wdata[0] = 0x03;		 // 读取数据指令0x03
	wdata[1] = (addr) >> 16; // 读取数据的地址
	wdata[2] = (addr) >> 8;	 // 读取数据的地址
	wdata[3] = (addr) >> 0;	 // 读取数据的地址

	W25Q64_WaitBusy();		   // 等待W25Q64空闲
	CS_ENABLE;				   // 打开CS
	SPI0_Write(wdata, 4);	   // 把wdata发送给W25Q64
	SPI0_Read(rbuff, datalen); // 读取datalen个字节数据
	CS_DISENABLE;			   // 关闭CS
}

///*-------------------------------------------------*/
///*函数名：擦除FLASH                                */
///*参  数：start_addr：擦除起始扇区   num：擦几个扇区*/
///*返回值：无                                       */
///*-------------------------------------------------*/
//void stm32_Erase_System_flash(uint32_t start_addr, uint16_t num)
//{
//	HAL_StatusTypeDef status;
//	FLASH_EraseInitTypeDef eraseInit;
//	uint32_t sectorError = 0;

//	// 先解锁FLASH
//	HAL_FLASH_Unlock();

//	// 配置擦除参数
//	eraseInit.TypeErase = FLASH_TYPEERASE_SECTORS;	// 扇区擦除类型
//	eraseInit.VoltageRange = FLASH_VOLTAGE_RANGE_3; // 电压范围：2.7V-3.6V
//	eraseInit.Sector = start_addr;					// 起始扇区
//	eraseInit.NbSectors = num;						// 要擦除的扇区数量

//	// 执行扇区擦除
//	status = HAL_FLASHEx_Erase(&eraseInit, &sectorError);

//	// 锁定FLASH
//	HAL_FLASH_Lock();

//	// 错误处理（可选）
//	if (status != HAL_OK)
//	{
//		// 擦除失败，可根据需要添加错误处理代码
//		// sectorError变量包含擦除失败的扇区编号
//	}
//}
/*---------------------------------------------------------------------*/
/*函数名：写入FLASH                                                    */
/*参  数：saddr：写入地址 wdata：写入数据指针  wnum：写入多少个字节    */
/*返回值：无                                                           */
/*---------------------------------------------------------------------*/
//void stm32_System_WriteFlash(uint32_t saddr, uint32_t *wdata, uint32_t wnum)
//{
//	HAL_StatusTypeDef status;

//	// 解锁FLASH
//	HAL_FLASH_Unlock();

//	// 循环写入数据，每次写入一个字（4字节）
//	while (wnum >= 4)
//	{
//		// 使用HAL库的FLASH写入函数，写入一个字
//		status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, saddr, *wdata);
//		if (status != HAL_OK)
//		{
//			// 写入失败，可根据需要添加错误处理代码
//			break;
//		}

//		wnum -= 4;	// 剩余字节数减4
//		saddr += 4; // 写入地址增加4
//		wdata++;	// 数据指针指向下一个字
//	}

//	// 处理剩余不足4字节的数据
//	if (wnum > 0)
//	{
//		// 读取当前地址的数据
//		uint32_t currentData = *(__IO uint32_t *)saddr;
//		uint32_t newData = currentData;
//		uint32_t byteOffset = 0;

//		// 逐个字节更新
//		while (wnum > 0)
//		{
//			newData &= ~(0xFF << (byteOffset * 8));
//			newData |= ((uint32_t)(*wdata) << (byteOffset * 8)) & (0xFF << (byteOffset * 8));

//			byteOffset++;
//			wnum--;

//			// 如果已经处理了一个完整的字，或者是最后一个字节
//			if (byteOffset == 4 || wnum == 0)
//			{
//				// 写入更新后的数据
//				status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, saddr, newData);
//				if (status != HAL_OK)
//				{
//					// 写入失败，可根据需要添加错误处理代码
//				}
//				break;
//			}
//		}
//	}

//	// 锁定FLASH
//	HAL_FLASH_Lock();
//}

/***************************************************************************** */
uint8_t wdata[256];
uint8_t Rdata[256];
void w25q64_test(void)
{
	W25Q64_Erase64K(0);
	HAL_Delay(50);
	uint16_t i, j;
	for (i = 0; i < 256; i++)
	{
		for (j = 0; j < 256; j++)
		{
			wdata[j] = i;
		}
		W25Q64_PageWrite(wdata, i);
	}
	HAL_Delay(50);
	for (i = 0; i < 256; i++)
	{
		W25Q64_Read(Rdata, i * 256, 256);
		for (j = 0; j < 256; j++)
		{
			printf("地址%d = %x\r\n", i * 256 + j, Rdata[j]);
		}
	}

}
