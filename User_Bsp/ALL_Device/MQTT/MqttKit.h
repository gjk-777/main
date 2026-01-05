#ifndef _MQTTKIT_H_
#define _MQTTKIT_H_


#include "Common.h"


//=============================配置==============================
//===========可以提供RTOS的内存管理方案，也可以使用C库的=========
//RTOS
#include <stdlib.h>

#define MQTT_MallocBuffer	malloc

#define MQTT_FreeBuffer		free
//==========================================================


#define MOSQ_MSB(A)         (uint8)((A & 0xFF00) >> 8)
#define MOSQ_LSB(A)         (uint8)(A & 0x00FF)


/*--------------------------------内存分配方案标志--------------------------------*/
#define MEM_FLAG_NULL		0
//大小在程序执行期间不变，可能是编译时分配的内存（如局部数组），
//也可能是动态分配后不再调整的内存，通常用于内存受限的环境或不需要调整的场景。
#define MEM_FLAG_ALLOC		1
//动态内存是在程序运行时，通过如 malloc、calloc、realloc 这样的函数从堆中分配的，开发者需要
//手动管理这些内存的释放。
#define MEM_FLAG_STATIC		2
//静态内存是在编译时分配的，大小和生命周期在程序执行前已经确定。
//典型的例子包括全局变量、静态局部变量。


typedef struct Buffer
{
	
	uint8	*_data;		//协议数据
	
	uint32	_len;		//写入的数据长度
	
	uint32	_size;		//缓存总大小
	
	uint8	_memFlag;	//内存使用的方案：0-未分配	1-使用的动态分配		2-使用的固定内存
	
} MQTT_PACKET_STRUCTURE;


/*--------------------------------固定头部消息类型--------------------------------*/
enum MqttPacketType
{
	
    MQTT_PKT_CONNECT = 1, /**< 连接请求数据包 */
    MQTT_PKT_CONNACK,     /**< 连接确认数据包 */
    MQTT_PKT_PUBLISH,     /**< 发布数据数据包 */
    MQTT_PKT_PUBACK,      /**< 发布确认数据包 */
    MQTT_PKT_PUBREC,      /**< 发布数据已接收数据包，Qos 2时，回复MQTT_PKT_PUBLISH */
    MQTT_PKT_PUBREL,      /**< 发布数据释放数据包， Qos 2时，回复MQTT_PKT_PUBREC */
    MQTT_PKT_PUBCOMP,     /**< 发布完成数据包， Qos 2时，回复MQTT_PKT_PUBREL */
    MQTT_PKT_SUBSCRIBE,   /**< 订阅数据包 */
    MQTT_PKT_SUBACK,      /**< 订阅确认数据包 */
    MQTT_PKT_UNSUBSCRIBE, /**< 取消订阅数据包 */
    MQTT_PKT_UNSUBACK,    /**< 取消订阅确认数据包 */
    MQTT_PKT_PINGREQ,     /**< ping 数据包 */
    MQTT_PKT_PINGRESP,    /**< ping 响应数据包 */
    MQTT_PKT_DISCONNECT,  /**< 断开连接数据包 */
	
	//新增
	
	MQTT_PKT_CMD  		 /**< 命令下发数据包 */
	
};


/*--------------------------------MQTT QOS等级--------------------------------*/
enum MqttQosLevel
{
	
    MQTT_QOS_LEVEL0,  /**< 最多发送一次 */
 /**特点: 消息最多发送一次，可能会丢失，不会有重传机制。
行为: 发布者发送消息后不会等待任何确认，即不保证消息一定能到达订阅者。如果网络不稳定或发生故障，消息可能会丢失。
使用场景: 适用于不重要的消息或传感器数据，比如环境温度数据，丢失一些数据不会有太大影响。 */	
	
    MQTT_QOS_LEVEL1,  /**< 最少发送一次  */
/**特点: 消息至少发送一次，可能会重复（即消息可能被多次接收）。
行为: 发布者发送消息后，要求接收方确认消息已收到。如果接收方没有及时确认，发布者会重发消息，直到收到确认为止。
这就可能导致消息被接收多次。
使用场景: 适用于需要确保消息到达但可以容忍重复的情况，比如一些状态更新通知。  */	
	
    MQTT_QOS_LEVEL2   /**< 只发送一次 */
/**特点: 消息保证只会到达一次，不会重复。
行为: 这是最高级别的消息传递保障。通过多步握手过程，确保消息在网络上传递的过程中不会重复，也不会丢失。
使用场景: 适用于不能容忍消息丢失或重复的场景，比如财务交易信息。  */	
};


/*--------------------------------MQTT 连接请求标志位，内部使用--------------------------------*/
enum MqttConnectFlag
{
	
    MQTT_CONNECT_CLEAN_SESSION  = 0x02,
    MQTT_CONNECT_WILL_FLAG      = 0x04,
    MQTT_CONNECT_WILL_QOS0      = 0x00,
    MQTT_CONNECT_WILL_QOS1      = 0x08,
    MQTT_CONNECT_WILL_QOS2      = 0x10,
    MQTT_CONNECT_WILL_RETAIN    = 0x20,
    MQTT_CONNECT_PASSORD        = 0x40,
    MQTT_CONNECT_USER_NAME      = 0x80
	
};


/*--------------------------------消息的packet ID，可自定义--------------------------------*/
#define MQTT_PUBLISH_ID			10

#define MQTT_SUBSCRIBE_ID		20

#define MQTT_UNSUBSCRIBE_ID		30


/*--------------------------------删包--------------------------------*/
void MQTT_DeleteBuffer(MQTT_PACKET_STRUCTURE *mqttPacket);

/*--------------------------------解包--------------------------------*/
uint8 MQTT_UnPacketRecv(uint8 *dataPtr);

/*--------------------------------登录组包--------------------------------*/
uint8 MQTT_PacketConnect(const int8 *user, const int8 *password, const int8 *devid,
						uint16 cTime, uint1 clean_session, uint1 qos,
						const int8 *will_topic, const int8 *will_msg, int32 will_retain,
						MQTT_PACKET_STRUCTURE *mqttPacket);

/*--------------------------------断开连接组包--------------------------------*/
uint1 MQTT_PacketDisConnect(MQTT_PACKET_STRUCTURE *mqttPacket);

/*--------------------------------连接响应解包--------------------------------*/
uint8 MQTT_UnPacketConnectAck(uint8 *rev_data);

/*--------------------------------数据点上传组包--------------------------------*/
uint1 MQTT_PacketSaveData(const int8 *pro_id, const char *dev_name,
								int16 send_len, int8 *type_bin_head, MQTT_PACKET_STRUCTURE *mqttPacket);

/*--------------------------------二进制文件上传组包--------------------------------*/
uint1 MQTT_PacketSaveBinData(const int8 *name, int16 file_len, MQTT_PACKET_STRUCTURE *mqttPacket);

/*--------------------------------命令下发解包--------------------------------*/
uint8 MQTT_UnPacketCmd(uint8 *rev_data, int8 **cmdid, int8 **req, uint16 *req_len);

/*--------------------------------命令回复组包--------------------------------*/
uint1 MQTT_PacketCmdResp(const int8 *cmdid, const int8 *req, MQTT_PACKET_STRUCTURE *mqttPacket);

/*--------------------------------订阅主题组包--------------------------------*/
uint8 MQTT_PacketSubscribe(uint16 pkt_id, enum MqttQosLevel qos, const int8 *topics[], uint8 topics_cnt, MQTT_PACKET_STRUCTURE *mqttPacket);

/*--------------------------------订阅主题回复解包--------------------------------*/
uint8 MQTT_UnPacketSubscribe(uint8 *rev_data);

/*--------------------------------取消订阅组包--------------------------------*/
uint8 MQTT_PacketUnSubscribe(uint16 pkt_id, const int8 *topics[], uint8 topics_cnt, MQTT_PACKET_STRUCTURE *mqttPacket);

/*--------------------------------取消订阅回复解包--------------------------------*/
uint1 MQTT_UnPacketUnSubscribe(uint8 *rev_data);

/*--------------------------------发布主题组包--------------------------------*/
uint8 MQTT_PacketPublish(uint16 pkt_id, const int8 *topic,
						const int8 *payload, uint32 payload_len,
						enum MqttQosLevel qos, int32 retain, int32 own,
						MQTT_PACKET_STRUCTURE *mqttPacket);

/*--------------------------------发布消息回复解包--------------------------------*/
uint8 MQTT_UnPacketPublish(uint8 *rev_data, int8 **topic, uint16 *topic_len, int8 **payload, uint16 *payload_len, uint8 *qos, uint16 *pkt_id);

/*--------------------------------发布消息的Ack组包--------------------------------*/
uint1 MQTT_PacketPublishAck(uint16 pkt_id, MQTT_PACKET_STRUCTURE *mqttPacket);

/*--------------------------------发布消息的Ack解包--------------------------------*/
uint1 MQTT_UnPacketPublishAck(uint8 *rev_data);

/*--------------------------------发布消息的Rec组包--------------------------------*/
uint1 MQTT_PacketPublishRec(uint16 pkt_id, MQTT_PACKET_STRUCTURE *mqttPacket);

/*--------------------------------发布消息的Rec解包--------------------------------*/
uint1 MQTT_UnPacketPublishRec(uint8 *rev_data);

/*--------------------------------发布消息的Rel组包--------------------------------*/
uint1 MQTT_PacketPublishRel(uint16 pkt_id, MQTT_PACKET_STRUCTURE *mqttPacket);

/*--------------------------------发布消息的Rel解包--------------------------------*/
uint1 MQTT_UnPacketPublishRel(uint8 *rev_data, uint16 pkt_id);

/*--------------------------------发布消息的Comp组包--------------------------------*/
uint1 MQTT_PacketPublishComp(uint16 pkt_id, MQTT_PACKET_STRUCTURE *mqttPacket);

/*--------------------------------发布消息的Comp解包--------------------------------*/
uint1 MQTT_UnPacketPublishComp(uint8 *rev_data);

/*--------------------------------心跳请求组包--------------------------------*/
uint1 MQTT_PacketPing(MQTT_PACKET_STRUCTURE *mqttPacket);


#endif
