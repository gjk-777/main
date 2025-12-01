#ifndef _ONENET_H_
#define _ONENET_H_


typedef struct
{

	_Bool Status;

} LED_INFO;





_Bool OneNet_DevLink(void);
void OneNet_SendData(void);
void OneNET_Subscribe(void);
void OneNet_RevPro(unsigned char *cmd);
#endif
