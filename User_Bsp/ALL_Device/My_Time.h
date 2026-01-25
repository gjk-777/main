
#ifndef __MY_TIME_H_
#define __MY_TIME_H_

#include "stdint.h"

void DateAndTime_Init(void);

void Get_Time(void);
void ReadTime(uint8_t *TimeArr);
void Timer_ReadTime(uint8_t *TimerArr, uint8_t *Timing_Flag);
void Write_Time(const uint8_t *Write_TimeArr);

#endif /* __TIME_H_ */
