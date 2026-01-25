/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    rtc.c
 * @brief   This file provides code for the configuration
 *          of the RTC instances.
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "rtc.h"

/* USER CODE BEGIN 0 */
#include "Time.h"
#include "usart.h"
/* USER CODE END 0 */

RTC_HandleTypeDef hrtc;

/* RTC init function */
void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef DateToUpdate = {0};

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */

  /** Initialize RTC Only
   */
  hrtc.Instance = RTC;
  hrtc.Init.AsynchPrediv = RTC_AUTO_1_SECOND;
  hrtc.Init.OutPut = RTC_OUTPUTSOURCE_NONE;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN Check_RTC_BKUP */

  /* USER CODE END Check_RTC_BKUP */

  /** Initialize RTC and set the Time and Date
   */
  sTime.Hours = 0x10;
  sTime.Minutes = 0x5;
  sTime.Seconds = 0x0;

  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }
  DateToUpdate.WeekDay = RTC_WEEKDAY_MONDAY;
  DateToUpdate.Month = RTC_MONTH_JANUARY;
  DateToUpdate.Date = 0x10;
  DateToUpdate.Year = 0x26;

  if (HAL_RTC_SetDate(&hrtc, &DateToUpdate, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */

  /* USER CODE END RTC_Init 2 */
}

void HAL_RTC_MspInit(RTC_HandleTypeDef *rtcHandle)
{

  if (rtcHandle->Instance == RTC)
  {
    /* USER CODE BEGIN RTC_MspInit 0 */

    /* USER CODE END RTC_MspInit 0 */
    HAL_PWR_EnableBkUpAccess();
    /* Enable BKP CLK enable for backup registers */
    __HAL_RCC_BKP_CLK_ENABLE();
    /* RTC clock enable */
    __HAL_RCC_RTC_ENABLE();
    /* USER CODE BEGIN RTC_MspInit 1 */

    /* USER CODE END RTC_MspInit 1 */
  }
}

void HAL_RTC_MspDeInit(RTC_HandleTypeDef *rtcHandle)
{

  if (rtcHandle->Instance == RTC)
  {
    /* USER CODE BEGIN RTC_MspDeInit 0 */

    /* USER CODE END RTC_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_RTC_DISABLE();
    /* USER CODE BEGIN RTC_MspDeInit 1 */

    /* USER CODE END RTC_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */
uint16_t MyRTC_Time[] = {2025, 12, 1, 20, 53, 6, 1}; // 联网获取时间后该数组会更新

void MyRTC_SetTime()
{
  time_t time_cnt;
  struct tm time_date; // 真实时间

  time_date.tm_hour = MyRTC_Time[3];
  time_date.tm_mday = MyRTC_Time[2];
  time_date.tm_min = MyRTC_Time[4];
  time_date.tm_mon = MyRTC_Time[1] - 1;
  time_date.tm_sec = MyRTC_Time[5];
  time_date.tm_year = MyRTC_Time[0] - 1900;

  time_cnt = mktime(&time_date) - 8 * 60 * 60; // 换算成时间戳秒数（UTC+8转UTC）

  // 添加调试信息
  Uart_printf(USART_DEBUG, "[RTC] 原始时间: %04d-%02d-%02d %02d:%02d:%02d\r\n",
              MyRTC_Time[0], MyRTC_Time[1], MyRTC_Time[2],
              MyRTC_Time[3], MyRTC_Time[4], MyRTC_Time[5]);
  Uart_printf(USART_DEBUG, "[RTC] 转换后时间戳(s): %lld\r\n", (long long)time_cnt);
  Uart_printf(USART_DEBUG, "[RTC] 写入CNTH: %04X\r\n", (uint16_t)((uint32_t)time_cnt >> 16));
  Uart_printf(USART_DEBUG, "[RTC] 写入CNTL: %04X\r\n", (uint16_t)((uint32_t)time_cnt & 0xFFFF));

  // 禁用RTC写保护
  __HAL_RTC_WRITEPROTECTION_DISABLE(&hrtc);

  // 设置RTC计数器的高位和低位字节
  hrtc.Instance->CNTH = (uint16_t)((uint32_t)time_cnt >> 16);
  hrtc.Instance->CNTL = (uint16_t)((uint32_t)time_cnt & 0xFFFF);

  // 启用RTC写保护
  __HAL_RTC_WRITEPROTECTION_ENABLE(&hrtc);

  // 验证写入是否成功
  uint16_t read_cnth = hrtc.Instance->CNTH;
  uint16_t read_cntl = hrtc.Instance->CNTL;
  uint32_t read_counter = ((uint32_t)read_cnth << 16) | read_cntl;
  Uart_printf(USART_DEBUG, "[RTC] 读取CNTH: %04X\r\n", read_cnth);
  Uart_printf(USART_DEBUG, "[RTC] 读取CNTL: %04X\r\n", read_cntl);
  Uart_printf(USART_DEBUG, "[RTC] 读取计数器: %08X (%lld)\r\n", read_counter, (long long)read_counter);
}
static time_t time_cnt;
static struct tm *time_date_ptr; // 真实时间指针
static struct tm time_date;      // 真实时间
void MyRTC_ReadTime()
{

  // 读取RTC计数器的高位和低位字节
  uint16_t read_cnth = hrtc.Instance->CNTH;
  uint16_t read_cntl = hrtc.Instance->CNTL;
  uint32_t rtc_counter = ((uint32_t)read_cnth << 16) | read_cntl;
  time_cnt = (time_t)rtc_counter + 8 * 60 * 60; // 读取RTC时间戳秒数（UTC转UTC+8）
  // 转换为本地时间
  time_date_ptr = localtime(&time_cnt);
  if (time_date_ptr != NULL)
  {
    time_date = *time_date_ptr;

    MyRTC_Time[3] = time_date.tm_hour;
    MyRTC_Time[2] = time_date.tm_mday;
    MyRTC_Time[4] = time_date.tm_min;
    MyRTC_Time[1] = time_date.tm_mon + 1;
    MyRTC_Time[5] = time_date.tm_sec;
    MyRTC_Time[0] = time_date.tm_year + 1900;
    MyRTC_Time[6] = time_date.tm_wday;
  }
  else
  {
    Uart_printf(USART_DEBUG, "[RTC] localtime转换失败\r\n");
  }
}

// 将网络获取的毫秒级时间戳转换为RTC需要的格式
static time_t time_cnt_Set;
void MyRTC_SetTimeFromTimestamp(long long timestamp_ms)
{

  // 1. 将毫秒级时间戳转换为秒级
  time_cnt_Set = timestamp_ms / 1000;
  // 2. 网络时间戳已经是UTC时间，直接写入RTC（无需时区转换，因为MyRTC_SetTime会进行UTC+8转UTC）
  // 禁用RTC写保护
  __HAL_RTC_WRITEPROTECTION_DISABLE(&hrtc);

  // 设置RTC计数器的高位和低位字节
  hrtc.Instance->CNTH = (uint16_t)((uint32_t)time_cnt_Set >> 16);
  hrtc.Instance->CNTL = (uint16_t)((uint32_t)time_cnt_Set & 0xFFFF);

  // 启用RTC写保护
  __HAL_RTC_WRITEPROTECTION_ENABLE(&hrtc);
}
/* USER CODE END 2 */

/* USER CODE END 1 */
