#ifndef _ESP8266_H_
#define _ESP8266_H_
#include "main.h"




#define REV_OK		1	//接收完成标志
#define REV_WAIT	0	//接收未完成标志


void ESP8266_Init(void);

void ESP8266_Clear(void);

_Bool ESP8266_SendCmd(char *cmd, char *res);

void ESP8266_SendData(unsigned char *data, unsigned short len);

unsigned char *ESP8266_GetIPD(unsigned short timeOut);
extern uint8_t chuan;;

#endif
