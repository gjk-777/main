/**
 ************************************************************
 ************************************************************
 ************************************************************
 *	文件名： 	onenet.c
 *
 *	作者： 		张继瑞
 *
 *	日期： 		2017-05-08
 *
 *	版本： 		V1.1
 *
 *	说明： 		与onenet平台的数据交互接口层
 *
 *	修改记录：	V1.0：协议封装、返回判断都在同一个文件，并且不同协议接口不同。
 *				V1.1：提供统一接口供应用层使用，根据不同协议文件来封装协议相关的内容。
 ************************************************************
 ************************************************************
 ************************************************************
 **/

// 单片机头文件

// 网络设备
#include "esp8266.h"

// 协议文件
#include "onenet.h"
#include "mqttkit.h"
#include "cJSON.h"

// 算法
#include "base64.h"
#include "hmac_sha1.h"

// 硬件驱动
#include "usart.h"
#include "led.h"
#include "buzzer.h"
// C库
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <float.h>
#include <limits.h>
#include <ctype.h>

#define PROID "5nPV5O180J"										  // 产品ID
#define ACCESS_KEY "QUZISTFGWjRCZmJHWWptdjREZ3NvbGpKYWo2QlZCQVc=" // 设备秘钥

#define DEVICE_NAME "d1" // 设备名称
char devid[16];
char key[48];
extern unsigned char esp8266_buf1_2[256], esp8266_buf3[256];
extern uint8_t humi;
extern uint8_t temp;

/*
************************************************************
*	函数名称：	OTA_UrlEncode
*
*	函数功能：	sign需要进行URL编码
*
*	入口参数：	sign：加密结果
*
*	返回参数：	0-成功	其他-失败
*
*	说明：		+			%2B
*				空格		%20
*				/			%2F
*				?			%3F
*				%			%25
*				#			%23
*				&			%26
*				=			%3D
************************************************************
*/
static unsigned char OTA_UrlEncode(char *sign)
{

	char sign_t[40];
	unsigned char i = 0, j = 0;
	unsigned char sign_len = strlen(sign);

	if (sign == (void *)0 || sign_len < 28)
		return 1;

	for (; i < sign_len; i++)
	{
		sign_t[i] = sign[i];
		sign[i] = 0;
	}
	sign_t[i] = 0;

	for (i = 0, j = 0; i < sign_len; i++)
	{
		switch (sign_t[i])
		{
		case '+':
			strcat(sign + j, "%2B");
			j += 3;
			break;

		case ' ':
			strcat(sign + j, "%20");
			j += 3;
			break;

		case '/':
			strcat(sign + j, "%2F");
			j += 3;
			break;

		case '?':
			strcat(sign + j, "%3F");
			j += 3;
			break;

		case '%':
			strcat(sign + j, "%25");
			j += 3;
			break;

		case '#':
			strcat(sign + j, "%23");
			j += 3;
			break;

		case '&':
			strcat(sign + j, "%26");
			j += 3;
			break;

		case '=':
			strcat(sign + j, "%3D");
			j += 3;
			break;

		default:
			sign[j] = sign_t[i];
			j++;
			break;
		}
	}

	sign[j] = 0;

	return 0;
}

/*
************************************************************
*	函数名称：	OTA_Authorization
*
*	函数功能：	计算Authorization
*
*	入口参数：	ver：参数组版本号，日期格式，目前仅支持格式"2018-10-31"
*				res：产品id
*				et：过期时间，UTC秒值
*				access_key：访问密钥
*				dev_name：设备名
*				authorization_buf：缓存token的指针
*				authorization_buf_len：缓存区长度(字节)
*
*	返回参数：	0-成功	其他-失败
*
*	说明：		当前仅支持sha1
************************************************************
*/
#define METHOD "sha1"
static char sign_buf[64];			  // 保存签名的Base64编码结果 和 URL编码结果
static char hmac_sha1_buf[64];		  // 保存签名
static char access_key_base64[64];	  // 保存access_key的Base64编码结合
static char string_for_signature[72]; // 保存string_for_signature，这个是加密的key
static unsigned char OneNET_Authorization(char *ver, char *res, unsigned int et, char *access_key, char *dev_name,
										  char *authorization_buf, unsigned short authorization_buf_len, _Bool flag)
{

	size_t olen = 0;

	//----------------------------------------------------参数合法性--------------------------------------------------------------------
	if (ver == (void *)0 || res == (void *)0 || et < 1564562581 || access_key == (void *)0 || authorization_buf == (void *)0 || authorization_buf_len < 120)
		return 1;

	//----------------------------------------------------将access_key进行Base64解码----------------------------------------------------
	memset(access_key_base64, 0, sizeof(access_key_base64));
	BASE64_Decode((unsigned char *)access_key_base64, sizeof(access_key_base64), &olen, (unsigned char *)access_key, strlen(access_key));
	//	Uart_printf(USART_DEBUG, "access_key_base64: %s\r\n", access_key_base64);

	//----------------------------------------------------计算string_for_signature-----------------------------------------------------
	memset(string_for_signature, 0, sizeof(string_for_signature));
	if (flag)
		snprintf(string_for_signature, sizeof(string_for_signature), "%d\n%s\nproducts/%s\n%s", et, METHOD, res, ver);
	else
		snprintf(string_for_signature, sizeof(string_for_signature), "%d\n%s\nproducts/%s/devices/%s\n%s", et, METHOD, res, dev_name, ver);
	//	Uart_printf(USART_DEBUG, "string_for_signature: %s\r\n", string_for_signature);

	//----------------------------------------------------加密-------------------------------------------------------------------------
	memset(hmac_sha1_buf, 0, sizeof(hmac_sha1_buf));

	hmac_sha1((unsigned char *)access_key_base64, strlen(access_key_base64),
			  (unsigned char *)string_for_signature, strlen(string_for_signature),
			  (unsigned char *)hmac_sha1_buf);

	//	Uart_printf(USART_DEBUG, "hmac_sha1_buf: %s\r\n", hmac_sha1_buf);

	//----------------------------------------------------将加密结果进行Base64编码------------------------------------------------------
	olen = 0;
	memset(sign_buf, 0, sizeof(sign_buf));
	BASE64_Encode((unsigned char *)sign_buf, sizeof(sign_buf), &olen, (unsigned char *)hmac_sha1_buf, strlen(hmac_sha1_buf));

	//----------------------------------------------------将Base64编码结果进行URL编码---------------------------------------------------
	OTA_UrlEncode(sign_buf);
	//	Uart_printf(USART_DEBUG, "sign_buf: %s\r\n", sign_buf);

	//----------------------------------------------------计算Token--------------------------------------------------------------------
	if (flag)
		snprintf(authorization_buf, authorization_buf_len, "version=%s&res=products%%2F%s&et=%d&method=%s&sign=%s", ver, res, et, METHOD, sign_buf);
	else
		snprintf(authorization_buf, authorization_buf_len, "version=%s&res=products%%2F%s%%2Fdevices%%2F%s&et=%d&method=%s&sign=%s", ver, res, dev_name, et, METHOD, sign_buf);
	//	Uart_printf(USART_DEBUG, "Token: %s\r\n", authorization_buf);

	return 0;
}
static MQTT_PACKET_STRUCTURE mqttPacket = {NULL, 0, 0, 0}; // 协议包
static unsigned char *dataPtr;
static char authorization_buf[160];
static _Bool status = 1;
_Bool OneNet_DevLink(void)
{
	OneNET_Authorization("2018-10-31", PROID, 1956499200, ACCESS_KEY, DEVICE_NAME,
						 authorization_buf, sizeof(authorization_buf), 0);
	// 函数的参数包括日期、产品 ID（PROID）、一个时间戳、访问密钥（ACCESS_KEY）、
	// 设备名称（DEVICE_NAME），以及用于存储生成的授权信息的缓冲区 authorization_buf。
	Uart_printf(USART_DEBUG, "OneNET_DevLink\r\n"
							 "NAME: %s,	PROID: %s,	KEY:%s\r\n",
				DEVICE_NAME, PROID, authorization_buf);

	if (MQTT_PacketConnect(PROID, authorization_buf, DEVICE_NAME, 1000, 1, MQTT_QOS_LEVEL0, NULL, NULL, 0, &mqttPacket) == 0)
	{
		ESP8266_SendData2(mqttPacket._data, mqttPacket._len); // 上传平台

		dataPtr = Connect_ONE_Returndata(250); // 等待平台响应
		if (dataPtr != NULL)
		{
			if (MQTT_UnPacketRecv(dataPtr) == MQTT_PKT_CONNACK)
			{
				switch (MQTT_UnPacketConnectAck(dataPtr))
				{
				case 0:
					Uart_printf(USART_DEBUG, "Tips:	连接成功\r\n");
					status = 0;
					break;

				case 1:
					Uart_printf(USART_DEBUG, "WARN:	连接失败：协议错误\r\n");
					break;
				case 2:
					Uart_printf(USART_DEBUG, "WARN:	连接失败：非法的clientid\r\n");
					break;
				case 3:
					Uart_printf(USART_DEBUG, "WARN:	连接失败：服务器失败\r\n");
					break;
				case 4:
					Uart_printf(USART_DEBUG, "WARN:	连接失败：用户名或密码错误\r\n");
					break;
				case 5:
					Uart_printf(USART_DEBUG, "WARN:	连接失败：非法链接(比如token非法)\r\n");
					break;

				default:
					Uart_printf(USART_DEBUG, "ERR:	连接失败：未知错误\r\n");
					break;
				}
			}
		}
		MQTT_DeleteBuffer(&mqttPacket); // 删包
	}
	else
		Uart_printf(USART_DEBUG, "WARN:	MQTT_PacketConnect Failed\r\n");
	return status;
}

uint8_t fenci;
extern float adc_mq2;
extern float adc_CO_MQ7;
/*********************12.1注释掉了，后面要打开******************/
unsigned char OneNet_FillBuf(char *buf)
{

	char text[48];
	memset(text, 0, sizeof(text));
	strcpy(buf, "{\"id\":\"123\",\"params\":{");
	memset(text, 0, sizeof(text));
	sprintf(text, "\"temp\":{\"value\":%d},", temp);
	strcat(buf, text);
	memset(text, 0, sizeof(text));
	sprintf(text, "\"humi\":{\"value\":%d},", humi);
	strcat(buf, text);
	memset(text, 0, sizeof(text));
	sprintf(text, "\"Mq2\":{\"value\":%.2f},", adc_mq2);
	strcat(buf, text);
	memset(text, 0, sizeof(text));
	sprintf(text, "\"CO\":{\"value\":%.2f},", adc_CO_MQ7);
	strcat(buf, text);

	memset(text, 0, sizeof(text));
	sprintf(text, "\"LED\":{\"value\":%s},", led_status ? "false" : "true");
	strcat(buf, text);
	memset(text, 0, sizeof(text));
	sprintf(text, "\"Beep\":{\"value\":%s}", beep_status ? "true" : "false");
	strcat(buf, text);
	memset(text, 0, sizeof(text));
	sprintf(text, ",\"Fire\":{\"value\":%s}", fire_status ? "true" : "false");
	strcat(buf, text);

	strcat(buf, "}}");

	return strlen(buf);
}

//==========================================================
//	函数名称：	OneNet_SendData
//
//	函数功能：	上传数据到平台
//
//	入口参数：	type：发送数据的格式
//
//	返回参数：	无
//
//	说明：
//==========================================================
void OneNet_SendData(void)
{

	MQTT_PACKET_STRUCTURE mqttPacket = {NULL, 0, 0, 0}; // 协议包

	char buf[512];

	short body_len = 0, i = 0;

	// Uart_printf(USART_DEBUG, "Tips:	OneNet_SendData-MQTT\r\n");

	memset(buf, 0, sizeof(buf)); // 用于初始化数组或结构体

	body_len = OneNet_FillBuf(buf); // 获取当前需要发送的数据流的总长度

	if (body_len)
	{
		if (MQTT_PacketSaveData(PROID, DEVICE_NAME, body_len, NULL, &mqttPacket) == 0) // 封包
		{
			for (; i < body_len; i++)
				mqttPacket._data[mqttPacket._len++] = buf[i];

			ESP8266_SendData(mqttPacket._data, mqttPacket._len); // 上传数据到平台
			// Uart_printf(USART_DEBUG, "Send %d Bytes\r\n", mqttPacket._len);

			MQTT_DeleteBuffer(&mqttPacket); // 删包
		}
		else
			Uart_printf(USART_DEBUG, "WARN:	EDP_NewBuffer Failed\r\n"); // qso1等级问题没回应会导致
	}
}

//==========================================================
//	函数名称：	OneNET_Publish
//
//	函数功能：	发布消息
//
//	入口参数：	topic：发布的主题
//				msg：消息内容
//
//	返回参数：	无
//
//	说明：
//==========================================================
void OneNET_Publish(const char *topic, const char *msg)
{

	MQTT_PACKET_STRUCTURE mqtt_packet = {NULL, 0, 0, 0}; // 协议包

	Uart_printf(USART_DEBUG, "Publish Topic: %s, Msg: %s\r\n", topic, msg);

	if (MQTT_PacketPublish(MQTT_PUBLISH_ID, topic, msg, strlen(msg), MQTT_QOS_LEVEL0, 0, 1, &mqtt_packet) == 0)
	{
		ESP8266_SendData2(mqtt_packet._data, mqtt_packet._len); // 向平台发送订阅请求

		MQTT_DeleteBuffer(&mqtt_packet); // 删包
	}
}

//==========================================================
//	函数名称：	OneNET_Subscribe
//	函数功能：	订阅
//	入口参数：	无
//	返回参数：	无
//==========================================================
static char topic_buf[56];
void OneNET_Subscribe(void)
{
	MQTT_PACKET_STRUCTURE mqtt_packet = {NULL, 0, 0, 0}; // 协议包

	const char *topic = topic_buf;

	snprintf(topic_buf, sizeof(topic_buf), "$sys/%s/%s/thing/property/set", PROID, DEVICE_NAME);

	Uart_printf(USART_DEBUG, "Subscribe Topic: %s\r\n", topic_buf);

	if (MQTT_PacketSubscribe(MQTT_SUBSCRIBE_ID, MQTT_QOS_LEVEL0, &topic, 1, &mqtt_packet) == 0)
	{
		ESP8266_SendData2(mqtt_packet._data, mqtt_packet._len); // 向平台发送订阅请求
		MQTT_DeleteBuffer(&mqtt_packet);						// 删包
	}
}

// int idd;

// void huiying()
//{
//	if(idd!=0)
//	{
//	char buf[100];
//	memset(buf, 0, sizeof(buf));
//	sprintf(buf, "{\"id\": \"%d\",\"code\": 200,\"msg\": \"success\"}", idd);
//	OneNET_Publish("$sys/vm5W2Mp1r4/d1/thing/property/set_reply",buf);
//	idd=0;
//	}
// }

//==========================================================
//	函数名称：	OneNet_RevPro
//
//	函数功能：	平台返回数据检测
//
//	入口参数：	dataPtr：平台返回的数据
//
//	返回参数：	无
//
//	说明：
//==========================================================

char led11[15] = "\"led\":true";
char led12[15] = "\"led\":false";

char led21[15] = "\"led2\":true";
char led22[15] = "\"led2\":false";

char led31[15] = "\"led3\":true";
char led32[15] = "\"led3\":false";

char led41[15] = "\"switch1\":true";
char led42[17] = "\"switch1\":false";

char she[10] = "shehumi";
uint8_t she_num, she_zhi, she_wei = 0;
char *zanshi = NULL;

static char *req_payload = NULL;
static char *cmdid_topic = NULL;
static unsigned short topic_len = 0;
static unsigned short req_len = 0;
static unsigned char qos = 0;
static  unsigned short pkt_id = 0;
static unsigned char type = 0;
static short result = 0;

void OneNet_RevPro(unsigned char *cmd)
{

	// cJSON *raw_json, *params_json, *led_json, *Beep_json;
	type = MQTT_UnPacketRecv(cmd);
	switch (type)
	{
	case MQTT_PKT_PUBLISH: // 接收的Publish消息
		result = MQTT_UnPacketPublish(cmd, &cmdid_topic, &topic_len, &req_payload, &req_len, &qos, &pkt_id);
		if (result == 0)
		{
			Uart_printf(USART_DEBUG, "----------------------------------------------");
			Uart_printf(USART_DEBUG, "topic: %s, topic_len: %d, payload: %s, payload_len: %d\r\n", cmdid_topic, topic_len, req_payload, req_len);
			// raw_json = cJSON_Parse(req_payload);				   // 解析JSON字符串
			// params_json = cJSON_GetObjectItem(raw_json, "params"); // 提取整个数组
			// led_json = cJSON_GetObjectItem(params_json, "LED");	   /// 提取对应属性
			// Beep_json = cJSON_GetObjectItem(params_json, "Beep");  /// 提取对应属性
			//  if (Beep_json != NULL)
			//  {
			//  	if (Beep_json->type == cJSON_True)
			//  	{
			//  		Uart_printf(USART_DEBUG, "Beep ON\r\n");
			//  		Bsp_LedON();
			//  	}
			//  	else
			//  	{
			//  		Uart_printf(USART_DEBUG, "Beep OFF\r\n");
			//  		Bsp_LedOFF();
			//  	}
			//  }
		}
		break;
	case MQTT_PKT_PUBACK: // 发送Publish消息，平台回复的Ack
		if (MQTT_UnPacketPublishAck(cmd) == 0)
			printf("Tips:	MQTT Publish Send OK\r\n");
		break;
	case MQTT_PKT_SUBACK: // 发送Subscribe消息的Ack
		if (MQTT_UnPacketSubscribe(cmd) == 0)
			printf("Tips:	MQTT Subscribe OK\r\n");
		else
			printf("Tips:	MQTT Subscribe Err\r\n");
		break;
	default:
		result = -1;
		break;
	}
	ESP8266_Clear3(); // 清空缓存
	if (result == -1)
		return;
	if (type == MQTT_PKT_CMD || type == MQTT_PKT_PUBLISH)
	{
		MQTT_FreeBuffer(cmdid_topic);
		MQTT_FreeBuffer(req_payload);
	}
}
