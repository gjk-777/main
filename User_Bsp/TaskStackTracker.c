/**
 * @file TaskStackTracker.c
 * @author Kelvin Hsu @Weilianzhikong (magic.zju@gmail.com)
 * @brief 任务堆栈信息跟踪
 * @version 1.0
 * @date 2021-12-13
 *
 * @copyright Copyright (c) 2021
 *
 */

#include "stddef.h"
#include "TaskStackTracker.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

/* 堆栈信息定义 */

#ifdef TASK_STACK_DEBUG
static uint16_t ScanPeriod;

typedef struct /// 任务堆栈信息
{
  char taskName[configMAX_TASK_NAME_LEN + 1]; ///< 任务名称
  uint16_t stackSize;                         ///< 任务分配的堆栈大小 单位“WORD”
  uint16_t free;                              ///< 任务堆栈高水位2，即剩余空间 单位“WORD”
  uint16_t suggest;                           ///< 预估任务需要堆栈空间大小 单位“WORD”
} t_taskDepthInfo;

static t_taskDepthInfo xTaskDepth[ESTIMATE_TASK_COUNT];
static HeapStats_t heapInfo;

/* 任务序号比较函数，用于任务列表排序 */
static int compareTasks(const void *a, const void *b)
{
  TaskStatus_t *taskA = (TaskStatus_t *)a;
  TaskStatus_t *taskB = (TaskStatus_t *)b;

  return taskA->xTaskNumber - taskB->xTaskNumber;
}

/**
 * @brief 周期扫描执行函数
 *
 */
void vgCheckTaskStack(void)
{
  TaskStatus_t *pxTaskStatusArray;
  volatile UBaseType_t uxArraySize, x;
  UBaseType_t uxHighWaterMark, uxStackSize;
  uint32_t ulTotalRunTime;
  /* 获取系统堆栈信息 */
  vPortGetHeapStats(&heapInfo);

  // 获取系统中任务的数量
  uxArraySize = uxTaskGetNumberOfTasks();

  // 为任务状态数组分配内存
  pxTaskStatusArray = pvPortMalloc(uxArraySize * sizeof(TaskStatus_t));

  if (pxTaskStatusArray != NULL)
  {
    // 获取所有任务的状态
    uxArraySize = uxTaskGetSystemState(pxTaskStatusArray, uxArraySize, &ulTotalRunTime);

    /* 对任务列表排序，按照任务序号进行 */
    qsort(pxTaskStatusArray, uxArraySize, sizeof(TaskStatus_t), compareTasks);
    printf("/************************************************/\r\n");
    // 遍历每个任务
    for (x = 0; x < uxArraySize && x < ESTIMATE_TASK_COUNT; x++)
    {
      uxHighWaterMark = uxTaskGetStackHighWaterMark(pxTaskStatusArray[x].xHandle);

      // 计算任务的堆栈大小
      uxStackSize = ((uint32_t)pxTaskStatusArray[x].xHandle - (uint32_t)pxTaskStatusArray[x].pxStackBase - 16) / 4;

      /* 任务名称拷贝 */
      if (strcmp(pxTaskStatusArray[x].pcTaskName, xTaskDepth[x].taskName) != 0)
      {
        strcpy(xTaskDepth[x].taskName, pxTaskStatusArray[x].pcTaskName);
      }

      /* 堆栈属性赋值 */
      xTaskDepth[x].stackSize = uxStackSize;
      xTaskDepth[x].free = uxHighWaterMark;
      xTaskDepth[x].suggest = uxStackSize - uxHighWaterMark;

      // 打印堆栈信息
      printf("Task:%d: %s, StackSize: %d, Free: %d, Suggest: %d\r\n", x,
             xTaskDepth[x].taskName,
             xTaskDepth[x].stackSize,
             xTaskDepth[x].free,
             xTaskDepth[x].suggest);
    }
    printf("剩余堆内存: %d 字节\n", xPortGetFreeHeapSize());
    printf("/************************************************/\r\n");
    // 释放之前分配的内存
    vPortFree(pxTaskStatusArray);
  }
}

/* 周期扫描任务 */
static void Task_Tracker_Task_Func(void *argument)
{
  for (;;)
  {

    vgCheckTaskStack(); /* 获取系统堆栈信息 */
    vTaskDelay(ScanPeriod);
  }
}
uint32_t Task_Tracker_GetFreeRoom(void)
{
  vPortGetHeapStats(&heapInfo); /* 获取系统堆栈信息 */
  return heapInfo.xAvailableHeapSpaceInBytes;
}

#endif /* TASK_STACK_DEBUG */

void Task_Tracker_Init(uint16_t period)
{
#ifdef TASK_STACK_DEBUG
  if (period < 50)
    return;
  ScanPeriod = period;
  xTaskCreate(Task_Tracker_Task_Func, "tracker", 128, NULL, 3, NULL);
#endif /* TASK_STACK_DEBUG */
}
