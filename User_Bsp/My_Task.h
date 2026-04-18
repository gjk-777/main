#ifndef __MY_TASK_H__
#define __MY_TASK_H__

#include "FreeRTOS.h"
#include "event_groups.h"

/* 事件组句柄 */
extern EventGroupHandle_t xAlarmEvent;

/* 报警事件位定义 */
#define EVENT_TEMP_BITS (0x01 << 0)
#define EVENT_MQ2_BITS (0x01 << 1)
#define EVENT_CO_MQ7_BITS (0x01 << 2)
#define EVENT_FIRE_BITS (0x01 << 3)
#define EVENT_BODY_BITS (0x01 << 4)

void My_Drivers_Init(void);

#endif
