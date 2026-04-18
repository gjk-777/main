#include "use_boot_flash.h"
#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"

#include "stdio.h"
#include "stddef.h"
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


/***********************************窗口属性Flash存储实现*********************************/
#include "led.h"

/**
 * @brief 计算校验和
 * @param data 数据指针
 * @param len 数据长度
 * @return 校验和
 */
static uint16_t Window_CalculateChecksum(uint8_t *data, uint16_t len)
{
    uint16_t checksum = 0;
    for (uint16_t i = 0; i < len; i++)
    {
        checksum += data[i];
    }
    return checksum;
}

/**
 * @brief 解锁Flash
 */
static void Window_Flash_Unlock(void)
{
    HAL_FLASH_Unlock();
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_WRPERR | FLASH_FLAG_PGERR);
}

/**
 * @brief 锁定Flash
 */
static void Window_Flash_Lock(void)
{
    HAL_FLASH_Lock();
}

/**
 * @brief 擦除指定Flash页
 * @param pageAddress 页起始地址
 * @return 0成功，1失败
 */
static uint8_t Window_Flash_ErasePage(uint32_t pageAddress)
{
    FLASH_EraseInitTypeDef EraseInitStruct;
    uint32_t pageError = 0;

    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.PageAddress = pageAddress;
    EraseInitStruct.NbPages = 1;

    if (HAL_FLASHEx_Erase(&EraseInitStruct, &pageError) != HAL_OK)
    {
        return 1;
    }
    return 0;
}

/**
 * @brief 将窗口角度保存到Flash
 * @param angle 窗户角度 (0-90度)
 * @return WindowFlashStatus_t 操作状态
 * 
 * 执行流程：
 * 1. 参数校验，角度限制在0-90度
 * 2. 构建WindowData_t结构体，填入魔数和角度
 * 3. 计算校验和
 * 4. 解锁Flash
 * 5. 擦除目标页
 * 6. 按字(32位)写入数据
 * 7. 锁定Flash
 * 
 * 注意：Flash写入必须按16位或32位对齐
 */
WindowFlashStatus_t Window_Flash_Save(uint8_t angle)
{
    // 角度限制
    if (angle > 90)
    {
        angle = 90;
    }

    // 构建数据结构
    WindowData_t windowData = {0};
    windowData.magic = WINDOW_DATA_MAGIC;
    windowData.window_angle = angle;
    windowData.reserved[0] = 0;
    windowData.reserved[1] = 0;
    windowData.reserved[2] = 0;
    
    // 计算校验和(不包括checksum字段本身)
    windowData.checksum = Window_CalculateChecksum((uint8_t *)&windowData, 
                                                    offsetof(WindowData_t, checksum));

    // 解锁Flash
    Window_Flash_Unlock();

    // 擦除页
    if (Window_Flash_ErasePage(WINDOW_FLASH_ADDR) != 0)
    {
        Window_Flash_Lock();
        printf("[Window Flash] Erase failed!\r\n");
        return WINDOW_FLASH_ERASE_ERROR;
    }

    // 写入数据(按32位字写入)
    uint32_t *dataPtr = (uint32_t *)&windowData;
    uint32_t wordCount = (sizeof(WindowData_t) + 3) / 4;  // 向上取整到字对齐

    for (uint32_t i = 0; i < wordCount; i++)
    {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, 
                              WINDOW_FLASH_ADDR + i * 4, 
                              dataPtr[i]) != HAL_OK)
        {
            Window_Flash_Lock();
            printf("[Window Flash] Write failed at word %d!\r\n", i);
            return WINDOW_FLASH_WRITE_ERROR;
        }
    }

    // 锁定Flash
    Window_Flash_Lock();

    printf("[Window Flash] Save success! Angle=%d\r\n", angle);
    return WINDOW_FLASH_OK;
}

/**
 * @brief 从Flash加载窗口角度
 * @param angle 输出参数，读取到的角度
 * @return WindowFlashStatus_t 操作状态
 * 
 * 执行流程：
 * 1. 从Flash地址读取WindowData_t结构体
 * 2. 验证魔数是否正确
 * 3. 验证校验和
 * 4. 返回角度值
 * 
 * 注意：直接从Flash读取不需要解锁
 */
WindowFlashStatus_t Window_Flash_Load(uint8_t *angle)
{
    WindowData_t *windowData = (WindowData_t *)WINDOW_FLASH_ADDR;

    // 检查魔数
    if (windowData->magic != WINDOW_DATA_MAGIC)
    {
        printf("[Window Flash] Invalid magic: 0x%08X (expected 0x%08X)\r\n", 
               windowData->magic, WINDOW_DATA_MAGIC);
        return WINDOW_FLASH_DATA_INVALID;
    }

    // 计算并验证校验和
    uint16_t calcChecksum = Window_CalculateChecksum((uint8_t *)windowData, 
                                                      offsetof(WindowData_t, checksum));
    if (calcChecksum != windowData->checksum)
    {
        printf("[Window Flash] Checksum mismatch: calc=0x%04X, stored=0x%04X\r\n", 
               calcChecksum, windowData->checksum);
        return WINDOW_FLASH_DATA_INVALID;
    }

    // 检查角度范围
    if (windowData->window_angle > 90)
    {
        printf("[Window Flash] Invalid angle: %d\r\n", windowData->window_angle);
        return WINDOW_FLASH_DATA_INVALID;
    }

    *angle = windowData->window_angle;
    printf("[Window Flash] Load success! Angle=%d\r\n", *angle);
    return WINDOW_FLASH_OK;
}

/**
 * @brief 窗口Flash初始化，上电时调用
 * 
 * 执行流程：
 * 1. 尝试从Flash读取窗口角度
 * 2. 如果数据有效，设置PWM恢复窗户状态
 * 3. 如果数据无效(首次上电或Flash损坏)，使用默认角度0
 * 
 * 注意：此函数应在PWM初始化之后调用
 */
void Window_Flash_Init(void)
{
    uint8_t angle = 0;
    WindowFlashStatus_t status = Window_Flash_Load(&angle);

    if (status == WINDOW_FLASH_OK)
    {
        // 数据有效，恢复窗户角度（不再重复写Flash，Flash中已有正确值）
        printf("[Window Flash] Restoring angle: %d\r\n", angle);
        Servo_angle_ex(angle, false);
    }
    else
    {
        // 数据无效(首次上电或Flash损坏)，使用默认值0
        printf("[Window Flash] Using default angle: 0\r\n");
        Servo_angle_ex(0, false);
        Window_Flash_Save(0); // 仅此处写一次Flash
    }
}

/***********************************阀门属性Flash存储实现*********************************/
/**
 * @brief 计算校验和（复用窗户的函数）
 */
// 使用上面定义的Window_CalculateChecksum函数

/**
 * @brief 将阀门角度保存到Flash
 * @param angle 阀门角度 (0-90度)
 * @return WindowFlashStatus_t 操作状态
 */
WindowFlashStatus_t Famen_Flash_Save(uint8_t angle)
{
    // 角度限制
    if (angle > 90)
    {
        angle = 90;
    }

    // 构建数据结构
    FamenData_t famenData = {0};
    famenData.magic = FAMEN_DATA_MAGIC;
    famenData.famen_angle = angle;
    famenData.reserved[0] = 0;
    famenData.reserved[1] = 0;
    famenData.reserved[2] = 0;

    // 计算校验和(不包括checksum字段本身)
    famenData.checksum = Window_CalculateChecksum((uint8_t *)&famenData,
                                                   offsetof(FamenData_t, checksum));

    // 解锁Flash
    Window_Flash_Unlock();

    // 擦除页
    if (Window_Flash_ErasePage(FAMEN_FLASH_ADDR) != 0)
    {
        Window_Flash_Lock();
        printf("[Famen Flash] Erase failed!\r\n");
        return WINDOW_FLASH_ERASE_ERROR;
    }

    // 写入数据(按32位字写入)
    uint32_t *dataPtr = (uint32_t *)&famenData;
    uint32_t wordCount = (sizeof(FamenData_t) + 3) / 4;  // 向上取整到字对齐

    for (uint32_t i = 0; i < wordCount; i++)
    {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,
                              FAMEN_FLASH_ADDR + i * 4,
                              dataPtr[i]) != HAL_OK)
        {
            Window_Flash_Lock();
            printf("[Famen Flash] Write failed at word %d!\r\n", i);
            return WINDOW_FLASH_WRITE_ERROR;
        }
    }

    // 锁定Flash
    Window_Flash_Lock();

    printf("[Famen Flash] Save success! Angle=%d\r\n", angle);
    return WINDOW_FLASH_OK;
}

/**
 * @brief 从Flash加载阀门角度
 * @param angle 输出参数，读取到的角度
 * @return WindowFlashStatus_t 操作状态
 */
WindowFlashStatus_t Famen_Flash_Load(uint8_t *angle)
{
    FamenData_t *famenData = (FamenData_t *)FAMEN_FLASH_ADDR;

    // 检查魔数
    if (famenData->magic != FAMEN_DATA_MAGIC)
    {
        printf("[Famen Flash] Invalid magic: 0x%08X (expected 0x%08X)\r\n",
               famenData->magic, FAMEN_DATA_MAGIC);
        return WINDOW_FLASH_DATA_INVALID;
    }

    // 计算并验证校验和
    uint16_t calcChecksum = Window_CalculateChecksum((uint8_t *)famenData,
                                                      offsetof(FamenData_t, checksum));
    if (calcChecksum != famenData->checksum)
    {
        printf("[Famen Flash] Checksum mismatch: calc=0x%04X, stored=0x%04X\r\n",
               calcChecksum, famenData->checksum);
        return WINDOW_FLASH_DATA_INVALID;
    }

    // 检查角度范围
    if (famenData->famen_angle > 90)
    {
        printf("[Famen Flash] Invalid angle: %d\r\n", famenData->famen_angle);
        return WINDOW_FLASH_DATA_INVALID;
    }

    *angle = famenData->famen_angle;
    printf("[Famen Flash] Load success! Angle=%d\r\n", *angle);
    return WINDOW_FLASH_OK;
}

/**
 * @brief 阀门Flash初始化，上电时调用
 */
void Famen_Flash_Init(void)
{
    uint8_t angle = 0;
    WindowFlashStatus_t status = Famen_Flash_Load(&angle);

    if (status == WINDOW_FLASH_OK)
    {
        // 数据有效，恢复阀门角度（不再重复写Flash，Flash中已有正确值）
        printf("[Famen Flash] Restoring angle: %d\r\n", angle);
        Famen_angle_ex(angle, false);
    }
    else
    {
        // 数据无效(首次上电或Flash损坏)，使用默认值0
        printf("[Famen Flash] Using default angle: 0\r\n");
        Famen_angle_ex(0, false);
        Famen_Flash_Save(0); // 仅此处写一次Flash
    }
}




