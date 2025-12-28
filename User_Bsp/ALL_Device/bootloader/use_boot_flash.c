#include "use_boot_flash.h"
#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"

#include "stdio.h"
#include "usart.h"

//enum
//{
//	FLASHIF_OK = 0,			   // 操作成功
//	FLASHIF_ERASEKO,		   //	擦除失败
//	FLASHIF_WRITINGCTRL_ERROR, //	写入控制错误
//	FLASHIF_WRITING_ERROR	   //	写入失败
//};

//enum
//{
//	FLASHIF_PROTECTION_NONE = 0,		   //	无保护
//	FLASHIF_PROTECTION_PCROPENABLED = 0x1, //	PCROP保护
//	FLASHIF_PROTECTION_WRPENABLED = 0x2,   //	WRP保护
//	FLASHIF_PROTECTION_RDPENABLED = 0x4,   //	RDP保护
//};

///**
// * @brief 解锁闪存进行写入访问
// *
// */
//void FLASH_If_UnlockInit(void)
//{
//	HAL_FLASH_Unlock();
//	/* Clear pending flags (if any) */
//	__HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_WRPERR |
//						   FLASH_FLAG_PGERR);
//}

///**
// * @brief 此功能用于擦除指定范围的Flash页面
// *
// * @param StartSector 闪存区的开始地址
// * @param EndSector 闪存区的结束地址
// * @return uint32_t 0: 成功擦除Flash页面
// *                  1: 发生错误
// */
//uint8_t FLASH_If_Erase(uint32_t StartPageAddress, uint32_t EndPageAddress)
//{
//	FLASH_EraseInitTypeDef EraseInitStruct;
//	uint32_t SectorError;

//	// 解锁FLASH存储器
//	FLASH_If_UnlockInit();

//	// 配置擦除范围
//	EraseInitStruct.TypeErase = TYPEERASE_PAGES;										 // 按扇区擦除
//	EraseInitStruct.PageAddress = StartPageAddress;										 // 设置起始页地址
//	EraseInitStruct.NbPages = (EndPageAddress - StartPageAddress) / STM32_PAGE_SIZE + 1; // 计算要擦除的页数

//	// 执行擦除操作
//	if (HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError) != HAL_OK)
//	{
//		// 擦除失败，锁定FLASH并返回错误
//		HAL_FLASH_Lock();
//		return 1;
//	}

//	// 擦除成功，锁定FLASH
//	HAL_FLASH_Lock();
//	return 0;
//}
///**
// * @brief 此函数在flash中写入一个数据缓冲区（数据是32位对齐的）
// *        注:写入数据缓冲区后，检查闪存内容。
// * @param FlashAddress 写入数据缓冲区的起始地址
// * @param Data 数据缓冲区上的指针
// * @param DataLength 数据缓冲区长度（单位为32位字）
// * @return uint32_t 0:数据成功写入闪存
// *                   2:闪存中写入的数据与预期的不同
// *                   3:在闪存中写入数据时出错
// */
//uint32_t FLASH_If_Write(uint32_t FlashAddress, uint32_t *Data, uint32_t DataLength)
//{
//	FLASH_If_UnlockInit();
//	for (uint32_t i = 0; i < DataLength; i++)
//	{
//		if (HAL_FLASH_Program(TYPEPROGRAM_WORD, FlashAddress, *(uint32_t *)(Data + i)) == HAL_OK)
//		{
//			/*检查写入值*/
//			if (*(uint32_t *)FlashAddress != *(uint32_t *)(Data + i))
//			{
//				/*闪存内容与SRAM内容不匹配*/
//				return (FLASHIF_WRITINGCTRL_ERROR);
//			}
//			/*增量闪存目标地址*/
//			FlashAddress += 4;
//		}
//		else
//		{
//			/*在闪存中写入数据时出错*/
//			return (FLASHIF_WRITING_ERROR);
//		}
//	}
//	HAL_FLASH_Lock();
//	/* 数据成功写入闪存 */
//	return (FLASHIF_OK);
//}

/*测试内部FLASH操作：成功*/
//uint32_t w1buff[1024];
//void FLASH_If_Test(void)
//{
//	for (uint32_t i = 0; i < 1024; i++)
//	{
//		w1buff[i] = 0x12654321;//4kb大小
//	}
//	FLASH_If_Erase(0x08000000 + 60 * 1024, 0x08000000 + 60 * 1024 + STM32_PAGE_SIZE * 4);//4页
//	FLASH_If_Write(0x08000000 + 60 * 1024, w1buff, 1024);

//	for (uint32_t i = 0; i < 1024; i++)
//	{
//		printf("%x\r\n", *(uint32_t *)(0x08000000 + 60 * 1024 + i * 4));
//	}
//}

/***********************************BOOT相关********************************* */

//UpDataA_CB UpDataA;	 // A区更新用到的结构体
//OTA_InfoCB OTA_Info; // 保存在24C02内的OTA信息相关的结构体

//// 创建事件组
//EventGroupHandle_t BootEventGroup = NULL;

//// BOOT事件组的位定义（使用32位统一格式）
//#define UPDATA_A_FLAG (0x00000001 << 0)	   // 需要更新A区标记
//#define IAP_XMODEMC_FLAG (0x00000001 << 1) // XMODEM协议第一阶段：发送大写C
//#define IAP_XMODEMD_FLAG (0x00000001 << 2) // XMODEM协议第二阶段：处理数据包
//#define SET_VERSION_FLAG (0x00000001 << 3) // 需要设置OTA版本号标记
//#define CMD_5_FLAG (0x00000001 << 4)	   // 需要处理命令5标记
//#define CMD5_XMODEM_FLAG (0x00000001 << 5) // 命令5的XMODEM处理：将程序数据写入W25Q64
//#define CMD_6_FLAG (0x00000001 << 6)	   // 需要处理命令6标记

//// 事件组所有位的掩码，用于清除所有事件位
//#define BOOT_EVENT_ALL_MASK (UPDATA_A_FLAG | IAP_XMODEMC_FLAG | IAP_XMODEMD_FLAG | \
//							 SET_VERSION_FLAG | CMD_5_FLAG | CMD5_XMODEM_FLAG | CMD_6_FLAG)

//static void BootLoader_Info(void)
//{
//	printf("\r\n");						  // 串口0输出信息
//	printf("[1]擦除A区\r\n");			  // 串口0输出信息
//	printf("[2]串口IAP下载A区程序\r\n");  // 串口0输出信息
//	printf("[3]设置OTA版本号\r\n");		  // 串口0输出信息
//	printf("[4]查询OTA版本号\r\n");		  // 串口0输出信息
//	printf("[5]向外部Flash下载程序\r\n"); // 串口0输出信息
//	printf("[6]使用外部Flash内程序\r\n"); // 串口0输出信息
//	printf("[7]重启\r\n");
//}

//static uint8_t Boot_Enter(uint8_t timeout)
//{
//	printf("%dms内，输入小写字母'b'进入BOOT命令行\r\n", timeout * 100);
//	while (timeout--)
//	{
//		HAL_Delay(100);
//		if (Uart1RxBuffer[0] == 'b')
//		{
//			return 1;
//		}
//	}
//	return 0;
//}
//typedef void (*pFunction)(void); // 这个函数指针指向的函数没有参数
//void JumpToApplication(uint32_t App_Addr)
//{
//	// 1. 检查应用程序地址是否有效
//	/*	 判断sp栈顶指针的范围是否合法，在对应型号的RAM控件范围内*/
//	if ((*(uint32_t *)App_Addr >= 0x20000000) && (*(uint32_t *)App_Addr <= 0x20004FFF))
//	{
//		// 2. 将中断向量表重定向到应用程序地址
//		SCB->VTOR = App_Addr;
//		// 3. 获取应用程序的栈顶地址和复位处理函数地址
//		pFunction app_reset_handler = (pFunction)(*(__IO uint32_t *)(App_Addr + 4));
//		__set_MSP(*(__IO uint32_t *)App_Addr);
//		// 4.清除使用的外设
//		HAL_DeInit();

//		// 5. 跳转到应用程序
//		app_reset_handler();
//	}
//	else
//	{
//		printf("跳转失败\r\n");
//	}
//}
//void Boot_Brance(void)
//{
//	if (Boot_Enter(20) == 0) // 如果20ms内没有收到b，进入BootLoader命令行
//	{
//		if (OTA_Info.OTA_flag == OTA_SET_FLAG)
//		{
//			printf("OTA更新\r\n");
//			// 清除所有事件位
//			xEventGroupClearBits(BootEventGroup, BOOT_EVENT_ALL_MASK);
//			// 设置需要更新A区标记+
//			xEventGroupSetBits(BootEventGroup, UPDATA_A_FLAG);
//			UpDataA.W25Q64_BlockNB = 0; // W25Q64_BlockNB等于0，表明是OTA要更新A区
//		}
//		else
//		{
//			printf("正常启动跳转A分区\r\n");
//			JumpToApplication(STM32_A_SADDR);
//		}
//	}
//	printf("进入BootLoade命令行\r\n");
//}


