#ifndef _ESP8266_H_
#define _ESP8266_H_
#include "main.h"




#define REV_OK		1	//接收完成标志
#define REV_WAIT	0	//接收未完成标志


void ESP8266_Init(void);

void ESP8266_Clear1_2(void);
void ESP8266_Clear3(void);

_Bool ESP8266_SendCmd(char *cmd, char *res);
_Bool ESP8266_SendCmd2(char *cmd, char *res);
void Connect(char *cmd, char *res);
void ESP8266_SendData(unsigned char *data, unsigned short len);
void ESP8266_SendData2(unsigned char *data, unsigned short len);

unsigned char *Connect_ONE_Returndata(unsigned short timeOut);
unsigned char *Get_xiafa_data(unsigned short timeOut);

_Bool fa_Connect_WaitRecive(void);
_Bool shou_WaitRecive(void);
_Bool ESP8266_WaitRecive_connect(void);
#endif
