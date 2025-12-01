/**
关键函数及变量解释：
1、
ESP8266_SendCmd：用来上传数据前，发送命令/ok

ESP8266_SendCmd2：用来初始化和连接云平台时，发送命令，因为此时屏蔽掉了其他任务，不可以用任务延时

接受云平台下发数据不需要指令发送函数, 无ESP8266_SendCmd3

2、
ESP8266_SendData：向云平台上传各项设备状态和检测参数数据包

ESP8266_SendData2：连接云平台和订阅主题时，向云平台发数据包

接受云平台下发数据不需要数据发送函数, 无ESP8266_SendData3

3、
fa_Connect_WaitRecive：用于连接云平台和上传数据时，等待串口接受完云平台发送的反馈信息
之所以发和connect用一个是因为上传书和连接云平台在我的代码里不会同时调用

shou_WaitRecive：用于处理云平台下发指令时，等待串口接受完云平台指令下发的数据

4、
ESP8266_Clear1_2：清空发送和连接反馈消息的缓存
ESP8266_Clear3：清空接受下发数据的数据缓存区中的数据

5、
Get_xiafa_data：获取平台下发的数据
Connect_ONE_Returndata：获取连接云平台时，平台返回的数据

**/


//网络设备驱动
#include "esp8266.h"

//硬件驱动
#include "usart.h"

//C库
#include <string.h>
#include <stdio.h>
#include "cmsis_os.h"
#include "onenet.h"
#define ESP8266_WIFI_INFO		"AT+CWJAP=\"iPhone\",\"zhang031007\"\r\n"


unsigned char esp8266_buf1_2[512],esp8266_buf3[512];
unsigned short esp8266_cnt = 0, esp8266_cntPre = 0;
unsigned short esp8266_cnt2 = 0, esp8266_cntPre2 = 0;
extern unsigned char esp8266_buf1_2[512],esp8266_buf3[512];
extern osThreadId_t Task_ShangyunHandle;
extern osThreadId_t Task_YshouHandle;
extern osThreadId_t Task_YfaHandle;
//==========================================================
//	函数名称：	ESP8266_Clear1_2
//
//	函数功能：	清空发送和连接反馈消息的缓存
//
//	入口参数：	无
//
//	返回参数：	无
//
//	说明：		
//==========================================================
void ESP8266_Clear1_2(void)
{

	memset(esp8266_buf1_2, 0, sizeof(esp8266_buf1_2));
	esp8266_cnt = 0;
}
//	函数名称：	ESP8266_Clear3
//
//	函数功能：	清空接受下发数据的数据缓存区中的数据
//
//	入口参数：	无
//
//	返回参数：	无
//
//	说明：
void ESP8266_Clear3(void)
{

	memset(esp8266_buf3, 0, sizeof(esp8266_buf3));
	esp8266_cnt2 = 0;

}
//==========================================================
//	函数名称：	fa_Connect_WaitRecive
//
//	函数功能：	用于连接云平台和上传数据时，等待云平台发送的反馈信息接受完成
//
//	入口参数：	无
//
//	返回参数：	REV_OK-接收完成		REV_WAIT-接收超时未完成
//
//	说明：		循环调用检测是否接收完成
/*fa_Connect_WaitRecive：用于连接云平台和上传数据时，等待串口接受完云平台发送的反馈信息
之所以发和connect用一个是因为上传书和连接云平台在我的代码里不会同时调用
*/
//==========================================================
uint8_t error_num;
_Bool fa_Connect_WaitRecive(void)
{
	if(esp8266_cnt == 0) 							//如果接收计数为0 则说明没有处于接收数据中，所以直接跳出，结束函数
		return REV_WAIT;
	if(esp8266_cnt == esp8266_cntPre&&esp8266_cnt>50)				//如果上一次的值和这次相同，则说明接收完毕
	{
		if(strstr((char*)esp8266_buf1_2, ">"))
		{
			ESP8266_Clear1_2();
			esp8266_cnt=2;
			sprintf((char*)esp8266_buf1_2, ">");
			return REV_OK;	
		}
		esp8266_cnt = 0;							//清0接收计数
		ESP8266_Clear1_2();
		return REV_WAIT;								//下发指令，忽略
	}
	else if(esp8266_cnt == esp8266_cntPre&&esp8266_cnt<50)
	{
		/*
		如果你发送 AT+CIPSEND 指令并且连接已经断开，ESP8266 会在发送数据时检测到连接失效，
		并返回 ERROR 或 
		SEND FAIL。这是因为在发送数据时，ESP8266 尝试使用一个无效的 TCP 连接。
		正好每1秒就要用这个指令上传数据，利用这个机制，就可以不断的检测连接状态。
		*/	
		if(strstr((char*)esp8266_buf1_2, "ERROR"))
		{
			error_num++;
			if(error_num > 3)
			{
				vTaskResume(Task_ShangyunHandle);
			}
		}
		esp8266_cnt = 0;
		return REV_OK;								//返回接收完成标志		
	}
	esp8266_cntPre = esp8266_cnt;					//置为相同
	return REV_WAIT;								//返回接收未完成标志	
}
//shou_WaitRecive：用于处理云平台下发指令时，等待串口接受完云平台指令下发的数据

_Bool shou_WaitRecive(void)
{

	if(esp8266_cnt2 == 0) 							//如果接收计数为0 则说明没有处于接收数据中，所以直接跳出，结束函数
		return REV_WAIT;
		
	if(esp8266_cnt2 == esp8266_cntPre2&&esp8266_cnt2>50)				//如果上一次的值和这次相同，则说明接收完毕
	{
		esp8266_cnt2 = 0;							//清0接收计数

		return REV_OK;								//返回接收esp8266_buf3完成标志
	}
	else if(esp8266_cnt2 == esp8266_cntPre2&&esp8266_cnt2<50)
	{
		ESP8266_Clear3();
		esp8266_cnt2 = 0;
	}
	esp8266_cntPre2 = esp8266_cnt2;					//置为相同
	return REV_WAIT;								//返回接收未完成标志

}



//==========================================================
//	函数名称：	ESP8266_SendCmd
//
//	函数功能：	用来上传数据前，发送命令
//
//	入口参数：	cmd：命令
//				res：需要检查的返回指令
//
//	返回参数：	0-成功	1-失败
//
//	说明：		
//==========================================================
_Bool ESP8266_SendCmd(char *cmd, char *res)
{
	
	unsigned char timeOut = 4;
	HAL_UART_Transmit(&huart2, (unsigned char *)cmd, strlen((const char *)cmd), 5000);
	
	while(timeOut--)
	{
		if(fa_Connect_WaitRecive() == REV_OK)							//如果收到数据
		{
			if(strstr((const char *)esp8266_buf1_2, res) != NULL)		//如果检索到关键词
			{
				
				ESP8266_Clear1_2();									//清空缓存			
				return 0;
			}
		}
		vTaskDelay(pdMS_TO_TICKS(5));
	}
	return 1;
}
//==========================================================
//	函数名称：	ESP8266_SendCmd2
//
//	函数功能：	用来初始化和连接云平台时，发送命令，因为此时屏蔽掉了其他任务，所以不可以用任务延时
//
//	入口参数：	cmd：命令
//				res：需要检查的返回指令
//
//	返回参数：	0-成功	1-失败
//
//	说明：	
_Bool ESP8266_SendCmd2(char *cmd, char *res)
{
	
	unsigned char timeOut = 8;

	HAL_UART_Transmit(&huart2, (unsigned char *)cmd, strlen((const char *)cmd), 5000);
	
	while(timeOut--)
	{
		if(fa_Connect_WaitRecive() == REV_OK)							//如果收到数据
		{
			Uart_printf(USART_DEBUG, "%s\r\n", esp8266_buf1_2);
			if(strstr((const char *)esp8266_buf1_2, res) != NULL)		//如果检索到关键词
			{
				ESP8266_Clear1_2();									//清空缓存			
				return 0;
			}
		}
		HAL_Delay(5);
	}
	return 1;
}



//==========================================================
//	函数名称：	ESP8266_SendData
//
//	函数功能：	向云平台上传各项设备状态和检测参数数据包
//
//	入口参数：	data：数据
//				len：长度
//
//	返回参数：	无
//
//	说明：	此函数是通过AT指令将数据发送到onenet平台上的
//==========================================================

void ESP8266_SendData(unsigned char *data, unsigned short len)
{

	char cmdBuf[32];
	
	ESP8266_Clear1_2();							//清空接收缓存
	sprintf(cmdBuf, "AT+CIPSEND=%d\r\n", len);		//发送命令
	if(!ESP8266_SendCmd(cmdBuf, ">"))				//收到‘>’时可以发送数据
	{
		HAL_UART_Transmit(&huart2, data, len, 5000);		//发送设备连接请求数据
	}
}
//==========================================================
//	函数名称：	ESP8266_SendData
//
//	函数功能：	连接云平台和订阅主题时，向云平台发数据包
//
//	入口参数：	data：数据
//				len：长度
//
//	返回参数：	无
//
//	说明：	此函数是通过AT指令将数据发送到onenet平台上的
//==========================================================
void ESP8266_SendData2(unsigned char *data, unsigned short len)
{

	char cmdBuf[32];
	
	ESP8266_Clear1_2();								//清空接收缓存
	sprintf(cmdBuf, "AT+CIPSEND=%d\r\n", len);		//发送命令
	if(!ESP8266_SendCmd2(cmdBuf, ">"))				//收到‘>’时可以发送数据
	{
		HAL_UART_Transmit(&huart2, data, len, 5000);		//发送设备连接请求数据
	}
}

//==========================================================
//	函数名称：	Get_xiafa_data
//							ESP8266_GetIPD
//	函数功能：	获取平台下发的数据
//
//	入口参数：	等待的时间(乘以10ms)
//
//	返回参数：	平台返回的原始数据
//
//	说明：		不同网络设备返回的格式不同，需要去调试
//				如ESP8266的返回格式为	"+IPD,x:yyy"	x代表数据长度，yyy是数据内容
/*onent云平台通过无线网络转发给esp8266，esp8266通过串口发给stm32*/
//==========================================================
unsigned char *Get_xiafa_data(unsigned short timeOut)
{

	char *ptrIPD = NULL;
	
	do
	{
		if(shou_WaitRecive() == REV_OK)								//如果接收完成
		{
			ptrIPD = strstr((char *)esp8266_buf3, "IPD,");				//搜索“IPD”头
			if(ptrIPD == NULL)											//如果没找到，可能是IPD头的延迟，还是需要等待一会，但不会超过设定的时间
			{
				//Uart_printf(USART_DEBUG, "\"IPD\" not found\r\n");
			}
			else
			{
				ptrIPD = strchr(ptrIPD, ':');							//找到':'
				if(ptrIPD != NULL)
				{
					ptrIPD++;//应该是为了将指针头右移到需要处理的数据，然后传到下一个参数
									//指针p向后移动一位（ptr++），以便继续查找下一个匹配
					return (unsigned char *)(ptrIPD);
				}
				else
				{
					return NULL;
				}					
			}
		}
		
		vTaskDelay(pdMS_TO_TICKS(5));													//延时等待
	} while(timeOut--);	
	return NULL;														//超时还未找到，返回空指针

}
//==========================================================
//	函数名称：	Connect_ONE_Returndata
//
//	函数功能：	获取连接云平台时，平台返回的数据
//
//	入口参数：	等待的时间(乘以10ms)
//
//	返回参数：	平台返回的原始数据
//
//	说明：		不同网络设备返回的格式不同，需要去调试
//				如ESP8266的返回格式为	"+IPD,x:yyy"	x代表数据长度，yyy是数据内容
//==========================================================
unsigned char *Connect_ONE_Returndata(unsigned short timeOut)
{

	char *ptrIPD = NULL;
	
	do
	{
		if(fa_Connect_WaitRecive() == REV_OK)								//如果接收完成
		{
			ptrIPD = strstr((char *)esp8266_buf1_2, "IPD,");				//搜索“IPD”头
			if(ptrIPD == NULL)											//如果没找到，可能是IPD头的延迟，还是需要等待一会，但不会超过设定的时间
			{
				//Uart_printf(USART_DEBUG, "\"IPD\" not found\r\n");
			}
			else
			{
				ptrIPD = strchr(ptrIPD, ':');							//找到':'
				if(ptrIPD != NULL)
				{
					ptrIPD++;
					return (unsigned char *)(ptrIPD);
				}
				else
				{
					return NULL;
				}				
			}
		}
		HAL_Delay(5);
												//延时等待
	} while(timeOut--);	
	return NULL;														//超时还未找到，返回空指针
}

void Connect(char *cmd, char *res)
{
	uint8_t timeOut1=6;
	HAL_UART_Transmit(&huart2, (unsigned char *)cmd, strlen((const char *)cmd), 5000);
	while((fa_Connect_WaitRecive() != REV_OK));//等待接收完毕
//	//发送的指令也会在esp8266_buf1_2接受之后再发回来，本想清楚，但是当第一次接受完毕之后只接受了A
//	//而且根本没必要，因为是连续发的，程序中的接受完成会把所有的当成接受完成，但是不知道为啥会丢失
//	//A这个字母，但是不影响	
	Uart_printf(USART_DEBUG, "%s\r\n", esp8266_buf1_2);
	
	
	while(strstr((const char *)esp8266_buf1_2, res) == NULL)
	{
		timeOut1--;
		if(timeOut1==0)
		{
			HAL_NVIC_SystemReset();
		}
		HAL_Delay(1000);
	}
	ESP8266_Clear1_2();									//清空缓存	
}
void Connect2(char *cmd, char *res)
{
	uint8_t timeOut1=12;
	HAL_UART_Transmit(&huart2, (unsigned char *)cmd, strlen((const char *)cmd), 5000);
	while((fa_Connect_WaitRecive() != REV_OK));//等待接收完毕
//	//发送的指令也会在esp8266_buf1_2接受之后再发回来，本想清楚，但是当第一次接受完毕之后只接受了A
//	//而且根本没必要，因为是连续发的，程序中的接受完成会把所有的当成接受完成，但是不知道为啥会丢失
//	//A这个字母，但是不影响	
	HAL_Delay(50);
//	Uart_printf(USART_DEBUG, "wifi:%s\r\n", esp8266_buf1_2);
	while(strstr((const char *)esp8266_buf1_2, res) == NULL)
	{
		Uart_printf(USART_DEBUG, "wifi:%s\r\n", esp8266_buf1_2);
		timeOut1--;
		if(timeOut1==0)
		{
			HAL_NVIC_SystemReset();
		}
		HAL_Delay(500);
	}

	ESP8266_Clear1_2();									//清空缓存	
}

//==========================================================
//	函数名称：	ESP8266_Init
//
//	函数功能：	初始化ESP8266
//
//	入口参数：	无
//
//	返回参数：	无
//
//	说明：		
//==========================================================

void ESP8266_Init(void)
{
	
	HAL_Delay(500);
	ESP8266_Clear1_2();
	Uart_printf(USART_DEBUG, "1. ATE0\r\n");
	while(ESP8266_SendCmd2("ATE0\r\n", "OK"))
	HAL_Delay(500);	
//	Uart_printf(USART_DEBUG, "1. AT\r\n");
//	while(ESP8266_SendCmd2("AT\r\n", "OK"))
//	HAL_Delay(500);

//	Uart_printf(USART_DEBUG, "2. CWMODE\r\n");
//	while(ESP8266_SendCmd2("AT+CWMODE=1\r\n", "OK"))
//	HAL_Delay(100);
	
	Uart_printf(USART_DEBUG, "3. AT+CWLAPOPT=1,5\r\n");
	ESP8266_SendCmd2("AT+CWLAPOPT=1,5\r\n", "OK"); // 按信号强度排序
	
	HAL_Delay(500);
	Uart_printf(USART_DEBUG, "4. CWJAP\r\n");

	Connect2((char *)ESP8266_WIFI_INFO, "GOT IP");

/*	
	//之前担心连接wifi需要等的时间长，在while循环中一直调用AT指令连接wifi导致busy，但有更容易的方法
	//，其实之前改个ESP8266_SendCmd2里的timeout就行了，改大点，现在懒得改回去,但是这个
	//好的一点是连接速度快，因为如果加了timeout，会等的时间太久，可能是间隔设的不对，以后再试	
	while(ESP8266_SendCmd2(ESP8266_WIFI_INFO, "GOT IP"))
	{	
		if(k == 2)
		{
			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
			HAL_Delay(500);
			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
		}
		k++;
		HAL_Delay(1000);				
	}
	 k = 0;
	*/


	Uart_printf(USART_DEBUG, "5. ESP8266 Init OK\r\n");
}
