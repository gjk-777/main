
#include "m24c02.h"
#include "main.h"
#include "string.h"
#include "delay.h"

/* -------------------------------------------------------------------------------------------- */
/* ↓↓↓  I2C驱动函数************************/
/* ---------------------------------------------------------------------------------- */
/*-------------------------------------------------*/
/*函数名：IIC起始信号                              */
/*参  数：无                                       */
/*返回值：无                                       */
/*-------------------------------------------------*/
void IIC_Start(void)
{
	IIC_SCL_H; // 拉高SCL
	IIC_SDA_H; // 拉高SDA
	udelay(2); // 延时
	IIC_SDA_L; // 拉低SDA
	udelay(2); // 延时
	IIC_SCL_L; // 拉低SCL
}
/*-------------------------------------------------*/
/*函数名：IIC停止信号                              */
/*参  数：无                                       */
/*返回值：无                                       */
/*-------------------------------------------------*/
void IIC_Stop(void)
{
	IIC_SCL_H; // 拉高SCL
	IIC_SDA_L; // 拉低SDA
	udelay(2); // 延时
	IIC_SDA_H; // 拉高SDA
}
/*-------------------------------------------------*/
/*函数名：IIC发送一个字节数据                      */
/*参  数：txd：要发送的数据                        */
/*返回值：无                                       */
/*-------------------------------------------------*/
void IIC_Send_Byte(uint8_t txd)
{
	int8_t i; // 用于for循环

	for (i = 7; i >= 0; i--)
	{					  // 一个字节循环8次 从BIT7~BIT0
		IIC_SCL_L;		  // 拉低SCL
		if (txd & BIT(i)) // 判断当前需要发送的二进制，是1的话进入if分支
			IIC_SDA_H;	  // 拉高SDA
		else			  // 判断当前需要发送的二进制，是0的话进入else分支
			IIC_SDA_L;	  // 拉低SDA
		udelay(2);		  // 延时
		IIC_SCL_H;		  // 拉高SCL
		udelay(2);		  // 延时
	}
	IIC_SCL_L; // 拉低SCL
	IIC_SDA_H; // 拉高SDA，等待从机应答
}
/*-------------------------------------------------*/
/*函数名：IIC等待应答                              */
/*参  数：timeout：超时时间                        */
/*返回值：0：正确  其他：错误                      */
/*-------------------------------------------------*/
uint8_t IIC_Wait_Ack(int16_t timeout)
{
	do
	{			   // 循环等待
		timeout--; // 超时时间减1
		udelay(2); // 延时
	} while ((READ_SDA) && (timeout >= 0)); // 2个条件退出while循环，SDA变低从机应答 / 超时时间小于0
	if (timeout < 0)
		return 1; // 如果超时时间小于0，说明没有等到从机应答，返回1，表示错误
	IIC_SCL_H;	  // 等到从机应答，拉高SCL
	udelay(2);	  // 延时
	if (READ_SDA != 0)
		return 2; // 再次判断，如果不是稳定的低电平，返回2，表示错误
	IIC_SCL_L;	  // 拉低SCL
	udelay(2);	  // 延时
	return 0;	  // 正确，返回0
}
/*-------------------------------------------------*/
/*函数名：IIC接收一个字节数据                      */
/*参  数：ack：1 应答从机  0 不应答从机            */
/*返回值：接收的数据                               */
/*-------------------------------------------------*/
uint8_t IIC_Read_Byte(uint8_t ack)
{
	int8_t i;	 // 用于for循环
	uint8_t rxd; // 用于保存接收的数据

	rxd = 0; // 清零rxd
	for (i = 7; i >= 0; i--)
	{					   // 一个字节循环8次 从BIT7~BIT0
		IIC_SCL_L;		   // 拉低SCL
		udelay(2);		   // 延时
		IIC_SCL_H;		   // 拉高SCL
		if (READ_SDA)	   // 判断当前SDA线状态，如果是高电平，进入if
			rxd |= BIT(i); // 相应的二进位记录高电平1
		udelay(2);		   // 延时
	}
	IIC_SCL_L; // 拉低SCL
	udelay(2); // 延时
	if (ack)
	{			   // 如果ack是1，进入if
		IIC_SDA_L; // 拉低SDA
		IIC_SCL_H; // 拉高SCL
		udelay(2); // 延时
		IIC_SCL_L; // 拉低SCL
		IIC_SDA_H; // 拉高SDA
		udelay(2); // 延时
	}
	else
	{			   // 如果ack是0，进入else
		IIC_SDA_H; // 拉高SDA
		IIC_SCL_H; // 拉高SCL
		udelay(2); // 延时
		IIC_SCL_L; // 拉低SCL
		udelay(2); // 延时
	}
	return rxd; // 返回接收的数据rxd
}

/*-------------------------------------------------*/
/*函数名：24C02写入一字节数据                      */
/*参  数：addr：地址  wdata：需要写入的数据        */
/*返回值：0：正确 其他：错误                       */
/*-------------------------------------------------*/
uint8_t M24C02_WriteByte(uint8_t addr, uint8_t wdata)
{
	IIC_Start();				 // 发送起始信号
	IIC_Send_Byte(M24C02_WADDR); // 发送24C02写操作器件地址
	if (IIC_Wait_Ack(100) != 0)
		return 1;		 // 等待应答，错误的话，返回1
	IIC_Send_Byte(addr); // 发送内部存储空间地址
	if (IIC_Wait_Ack(100) != 0)
		return 2;		  // 等待应答，错误的话，返回2
	IIC_Send_Byte(wdata); // 发送数据
	if (IIC_Wait_Ack(100) != 0)
		return 3; // 等待应答，错误的话，返回3
	IIC_Stop();	  // 发送停止信号
	return 0;	  // 正确，返回0
}
/*-------------------------------------------------*/
/*函数名：24C02写入一页（16字节）数据              */
/*参  数：addr：地址  wdata：需要写入的数据指针    */
/*返回值：0：正确 其他：错误                       */
/*-------------------------------------------------*/
uint8_t M24C02_WritePage(uint8_t addr, uint8_t *wdata)
{

	uint8_t i; // 用于for循环

	IIC_Start();				 // 发送起始信号
	IIC_Send_Byte(M24C02_WADDR); // 发送24C02写操作器件地址
	if (IIC_Wait_Ack(100) != 0)
		return 1;		 // 等待应答，错误的话，返回1
	IIC_Send_Byte(addr); // 发送内部存储空间地址
	if (IIC_Wait_Ack(100) != 0)
		return 2; // 等待应答，错误的话，返回2
	for (i = 0; i < 16; i++)
	{							 // 循环16次写入一页
		IIC_Send_Byte(wdata[i]); // 发送数据
		if (IIC_Wait_Ack(100) != 0)
			return 3 + i; // 等待应答，错误的话，返回3+i
	}
	IIC_Stop(); // 发送停止信号
	return 0;	// 正确，返回0
}
/*---------------------------------------------------------*/
/*函数名：24C02读取数据                                    */
/*参  数：addr：地址  rdata：接收缓冲区 datalen:读取长度   */
/*返回值：0：正确 其他：错误                               */
/*---------------------------------------------------------*/
uint8_t M24C02_ReadData(uint8_t addr, uint8_t *rdata, uint16_t datalen)
{

	uint8_t i; // 用于for循环

	IIC_Start();				 // 发送起始信号
	IIC_Send_Byte(M24C02_WADDR); // 发送24C02写操作器件地址
	if (IIC_Wait_Ack(100) != 0)
		return 1;		 // 等待应答，错误的话，返回1
	IIC_Send_Byte(addr); // 发送内部存储空间地址
	if (IIC_Wait_Ack(100) != 0)
		return 2;				 // 等待应答，错误的话，返回2
	IIC_Start();				 // 再次发送起始信号
	IIC_Send_Byte(M24C02_RADDR); // 发送24C02读操作器件地址
	if (IIC_Wait_Ack(100) != 0)
		return 3; // 等待应答，错误的话，返回3
	for (i = 0; i < datalen - 1; i++)
	{								 // 循环datalen-1次，一个字节一个字节接收数据
		rdata[i] = IIC_Read_Byte(1); // 接收数据，发送应答给从机
	}
	rdata[datalen - 1] = IIC_Read_Byte(0); // 接收最后一个字节，不发送应答信号
	IIC_Stop();							   // 发送停止信号
	return 0;							   // 正确，返回0
}
/*-------------------------------------------------*/
/*函数名：读取24C02结构体到OTA_Info结构体缓冲区    */
/*参  数：无                                       */
/*返回值：无                                       */
/*-------------------------------------------------*/
// void M24C02_ReadOTAInfo(void)
//{
//	memset(&OTA_Info, 0, OTA_INFOCB_SIZE);					   // 清空OTA_Info结构体缓冲区
//	M24C02_ReadData(0, (uint8_t *)&OTA_Info, OTA_INFOCB_SIZE); // 从24C02读取数据，存放到OTA_Info结构体
// }
///*-------------------------------------------------*/
///*函数名：把OTA_Info结构体缓冲区数据保存到24C02    */
///*参  数：无                                       */
///*返回值：无                                       */
///*-------------------------------------------------*/
// void M24C02_WriteOTAInfo(void)
//{
//	uint8_t i;	   // 用于for循环
//	uint8_t *wptr; // uint8_t类型指针

//	wptr = (uint8_t *)&OTA_Info; // wptr指向OTA_Info结构体首地址
//	for (i = 0; i < OTA_INFOCB_SIZE / 16; i++)
//	{											 // 每次写入一页16个字节
//		M24C02_WritePage(i * 16, wptr + i * 16); // 写入一页数据
//		mdelay(5);								 // 延时
//	}
//}

uint8_t rbuff[256];

#define Page_16 1
// #define Byte_1
//  uint8_t wbuff[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
uint8_t wbuff[16] = {15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};

void M24C02_Test(void)
{
#ifdef Byte_1
	for (uint16_t i = 0; i < 256; i++)
	{
		M24C02_WriteByte(i, i);
		mdelay(5);
	}
#endif // Byte_1
#ifdef Page_16
	for (uint16_t i = 0; i < 16; i++)
	{
		M24C02_WritePage(i * 16, wbuff);
		mdelay(10);
	}
#endif
	M24C02_ReadData(0, rbuff, 256);
	for (uint16_t i = 0; i < 256; i++)
	{
		printf("地址：%d= %x\r\n", i, rbuff[i]);
	}
}
