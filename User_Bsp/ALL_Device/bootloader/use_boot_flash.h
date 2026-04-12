#ifndef USE_BOOT_FLASH_H
#define USE_BOOT_FLASH_H

#include "stdint.h"
#include "main.h"

//#define STM32_FLASH_SADDR 0x08000000                                          // FLASH起始地址
//#define STM32_PAGE_SIZE 1024                                                  // FLASH扇区大小
//#define STM32_PAGE_NUM 64                                                     // FLASH总扇区个数
//#define STM32_B_PAGE_NUM 20                                                   // B区扇区个数
//#define STM32_A_PAGE_NUM STM32_PAGE_NUM - STM32_B_PAGE_NUM                    // A区扇区个数 64-20=44
//#define STM32_A_START_PAGE STM32_B_PAGE_NUM                                   // A区起始扇区编号 20
//#define STM32_A_SADDR STM32_FLASH_SADDR + STM32_A_START_PAGE *STM32_PAGE_SIZE // A区起始地址

//#define OTA_SET_FLAG 0xAABB1122 // OTA_flag对勾状态对应的数值，如果OTA_flag等于该值，说明需要OTA更新A区

//typedef struct
//{
//    uint8_t Updatabuff[STM32_PAGE_SIZE]; // 更新A区时，用于保存从W25Q64中读取的数据
//    uint32_t W25Q64_BlockNB;             // 用于记录从哪个W25Q64的块中读取数据
//    uint32_t XmodemTimer;                // 用于记录Xmdoem协议第一阶段发送大写C 的时间间隔
//    uint32_t XmodemNB;                   // 用于记录Xmdoem协议接收到多少数据包了
//    uint32_t XmodemCRC;                  // 用于保存Xmdoem协议包计算的CRC16校验值
//} UpDataA_CB;

//typedef struct
//{
//    uint32_t OTA_flag;    // 标志性的变量，等于OTA_SET_FLAG定义的值时，表明需要OTA更新A区
//    uint32_t Firelen[11]; // W25Q64中不同块中程序固件的长度，0号成员固定对应W25Q64中编码0的块，用于OTA
//    uint8_t OTA_ver[32];
//} OTA_InfoCB;                              // OTA相关的信息结构体，需要保存到24C02
//#define OTA_INFOCB_SIZE sizeof(OTA_InfoCB) // OTA相关的信息结构体占用的字节长度

//extern OTA_InfoCB OTA_Info; // 外部变量声明
//extern UpDataA_CB UpDataA;  // 外部变量声明

//void FLASH_If_Test(void);
//uint8_t FLASH_If_Erase(uint32_t StartPageAddress, uint32_t EndPageAddress);

//uint32_t FLASH_If_Write(uint32_t FlashAddress, uint32_t *Data, uint32_t DataLength);

//void Boot_Brance(void);

/***********************************窗口属性Flash存储*********************************/
// STM32F103C8 Flash配置
#define WINDOW_FLASH_PAGE_SIZE    1024                                 // Flash页大小
#define WINDOW_FLASH_PAGE_NUM     63                                   // 使用第63页(最后一页)
#define WINDOW_FLASH_ADDR         (0x08000000 + WINDOW_FLASH_PAGE_NUM * WINDOW_FLASH_PAGE_SIZE) // 0x0800FC00

// 魔数，用于验证数据有效性
#define WINDOW_DATA_MAGIC         0x57444F57  // "WDOW" (Window)

// 窗口属性数据结构体
typedef struct
{
    uint32_t magic;           // 魔数，用于验证数据有效性
    uint8_t  window_angle;    // 窗户角度 (0-90度)
    uint8_t  reserved[3];     // 保留字节，4字节对齐
    uint16_t checksum;        // 校验和
} WindowData_t;

// Flash操作返回值
typedef enum
{
    WINDOW_FLASH_OK = 0,       // 操作成功
    WINDOW_FLASH_ERASE_ERROR,  // 擦除失败
    WINDOW_FLASH_WRITE_ERROR,  // 写入失败
    WINDOW_FLASH_READ_ERROR,   // 读取失败
    WINDOW_FLASH_DATA_INVALID  // 数据无效
} WindowFlashStatus_t;

// 函数声明
WindowFlashStatus_t Window_Flash_Save(uint8_t angle);
WindowFlashStatus_t Window_Flash_Load(uint8_t *angle);
void Window_Flash_Init(void);

/***********************************阀门属性Flash存储*********************************/
// 使用第62页存储阀门数据
#define FAMEN_FLASH_PAGE_NUM     62
#define FAMEN_FLASH_ADDR         (0x08000000 + FAMEN_FLASH_PAGE_NUM * WINDOW_FLASH_PAGE_SIZE) // 0x0800F800

// 阀门魔数
#define FAMEN_DATA_MAGIC         0x46414D45  // "FAME" (Famen)

// 阀门属性数据结构体
typedef struct
{
    uint32_t magic;           // 魔数，用于验证数据有效性
    uint8_t  famen_angle;     // 阀门角度 (0-90度)
    uint8_t  reserved[3];     // 保留字节，4字节对齐
    uint16_t checksum;        // 校验和
} FamenData_t;

// 阀门Flash操作函数声明
WindowFlashStatus_t Famen_Flash_Save(uint8_t angle);
WindowFlashStatus_t Famen_Flash_Load(uint8_t *angle);
void Famen_Flash_Init(void);

#endif
