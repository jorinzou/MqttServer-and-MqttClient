#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "socket_client.h"
#include "mqtt.h"

#define QOS_VALUE0 0
#define QOS_VALUE1 1
#define QOS_VALUE2 2

#define MQTT_DUP_FLAG     1<<3
#define MQTT_QOS0_FLAG    0<<1
#define MQTT_QOS1_FLAG    1<<1
#define MQTT_QOS2_FLAG    2<<1

#define MQTT_RETAIN_FLAG  1

#define MQTT_CLEAN_SESSION  1<<1
#define MQTT_WILL_FLAG      1<<2
#define MQTT_WILL_RETAIN    1<<5
#define MQTT_USERNAME_FLAG  1<<7
#define MQTT_PASSWORD_FLAG  1<<6

uint8 mqtt_num_rem_len_bytes(const uint8* buf) {
	uint8 num_bytes = 1;
	if ((buf[1] & 0x80) == 0x80) {
		num_bytes++;
		if ((buf[2] & 0x80) == 0x80) {
			num_bytes ++;
			if ((buf[3] & 0x80) == 0x80) {
				num_bytes ++;
			}
		}
	}
	return num_bytes;
}

uint16 mqtt_parse_rem_len(const uint8* buf) {
	uint16 multiplier = 1;
	uint16 value = 0;
	uint8 digit;
	
	buf++;

	do {
		digit = *buf;
		value += (digit & 127) * multiplier;
		multiplier *= 128;
		buf++;
	} while ((digit & 128) != 0);

	return value;
}

uint16 mqtt_parse_msg_id(const uint8* buf) {
	uint8 type = MQTTParseMessageType(buf);
	uint8 qos = MQTTParseMessageQos(buf);
	uint16 id = 0;
		
	if(type >= MQTT_MSG_PUBLISH && type <= MQTT_MSG_UNSUBACK) {
		if(type == MQTT_MSG_PUBLISH) {
			if(qos != 0) {
				// fixed header length + Topic (UTF encoded)
				// = 1 for "flags" byte + rlb for length bytes + topic size
				uint8 rlb = mqtt_num_rem_len_bytes(buf);
				uint8 offset = *(buf+1+rlb)<<8;	// topic UTF MSB
				offset |= *(buf+1+rlb+1);			// topic UTF LSB
				offset += (1+rlb+2);					// fixed header + topic size
				id = *(buf+offset)<<8;				// id MSB
				id |= *(buf+offset+1);				// id LSB
			}
		} else {
			// fixed header length
			// 1 for "flags" byte + rlb for length bytes
			uint8 rlb = mqtt_num_rem_len_bytes(buf);
			id = *(buf+1+rlb)<<8;	// id MSB
			id |= *(buf+1+rlb+1);	// id LSB
		}
	}
	return id;
}

uint16 mqtt_parse_pub_topic(const uint8* buf, uint8* topic) {
	const uint8* ptr;
	uint16 topic_len = mqtt_parse_pub_topic_ptr(buf, &ptr);
		
	if(topic_len != 0 && ptr != NULL) {
		memcpy(topic, ptr, topic_len);
	}
	
	return topic_len;
}

uint16 mqtt_parse_pub_topic_ptr(const uint8* buf, const uint8 **topic_ptr) {
	uint16 len = 0;
	
	if(MQTTParseMessageType(buf) == MQTT_MSG_PUBLISH) {
		// fixed header length = 1 for "flags" byte + rlb for length bytes
		uint8 rlb = mqtt_num_rem_len_bytes(buf);
		len = *(buf+1+rlb)<<8;	// MSB of topic UTF
		len |= *(buf+1+rlb+1);	// LSB of topic UTF
		// start of topic = add 1 for "flags", rlb for remaining length, 2 for UTF
		*topic_ptr = (buf + (1+rlb+2));
	} else {
		*topic_ptr = NULL;
	}
	return len;
}

uint16 mqtt_parse_publish_msg(const uint8* buf, uint8* msg) 
{
	const uint8* ptr;
	uint16 msg_len = mqtt_parse_pub_msg_ptr(buf, &ptr);
	if(msg_len != 0 && ptr != NULL) {
		memcpy(msg, ptr, msg_len);
	}
	return msg_len;
}

uint16 mqtt_parse_pub_msg_ptr(const uint8* buf, const uint8 **msg_ptr) {
	uint16 len = 0;
	
	//printf("mqtt_parse_pub_msg_ptr\n");
	
	if(MQTTParseMessageType(buf) == MQTT_MSG_PUBLISH) {
		// message starts at
		// fixed header length + Topic (UTF encoded) + msg id (if QoS>0)
		uint8 rlb = mqtt_num_rem_len_bytes(buf);
		uint8 offset = (*(buf+1+rlb))<<8;	// topic UTF MSB
		offset |= *(buf+1+rlb+1);			// topic UTF LSB
		offset += (1+rlb+2);				// fixed header + topic size

		if(MQTTParseMessageQos(buf)) {
			offset += 2;					// add two bytes of msg id
		}

		*msg_ptr = (buf + offset);
				
		// offset is now pointing to start of message
		// length of the message is remaining length - variable header
		// variable header is offset - fixed header
		// fixed header is 1 + rlb
		// so, lom = remlen - (offset - (1+rlb))
      	len = mqtt_parse_rem_len(buf) - (offset-(rlb+1));
	} else {
		*msg_ptr = NULL;
	}
	return len;
}

static uint8_t client_id[512] = {"mqtt_client"};
static uint8_t user_name[512] = {"mqtt"};
static uint8_t passwd[512] = {"12345678"};
#define KEEP_ALIVE 20
int mqtt_connect(int sockfd)
{
    uint8 flags = 0x00;
	uint8 *packet = NULL;
	uint16 packet_length = 0;	
	uint16 clientidlen = strlen(client_id);
	uint16 usernamelen = strlen(user_name);
	uint16 passwordlen = strlen(passwd);
	uint16 payload_len = clientidlen + 2;
	// Variable header
	uint8 var_header[10] = {
		0x00,0x04,/*len*/
        0x4d,0x51,0x54,0x54,/*mqtt*/
		0x04,/*协议版本*/
	};
	uint8 fixedHeaderSize = 2;    // Default size = one byte Message Type + one byte Remaining Length
	uint8 remainLen = 0;
	uint8 *fixed_header = NULL;
	uint16 offset = 0;
    
	// Preparing the flags
	if(usernamelen) {
        /*用户名长度(可选)*/
		payload_len += usernamelen + 2;
		flags |= MQTT_USERNAME_FLAG;/*或上用户名标记*/
	}
	if(passwordlen) {
        /*用户密码(可选)*/
		payload_len += passwordlen + 2;
		flags |= MQTT_PASSWORD_FLAG;/*用户密码标记位*/
	}
	flags |= MQTT_CLEAN_SESSION;

	var_header[7] = flags;/*连接标记*/
    /*保持连接字段，占用两个字节*/
	var_header[8] = KEEP_ALIVE>>8;
	var_header[9] = KEEP_ALIVE&0xFF;

    /*剩余长度，也就是可变报头加上负载的长度*/
	remainLen = sizeof(var_header)+payload_len;

	if (remainLen > 127) {
	    fixedHeaderSize++;// add an additional byte for Remaining Length          
	}
    /*固定报头*/
	fixed_header = (uint8 *)malloc(fixedHeaderSize);

	// Message Type
	*fixed_header = MQTT_MSG_CONNECT;/*报文类型,connect*/

	// Remaining Length,剩余长度计算，可变长编码
	if (remainLen <= 127) {
	    *(fixed_header+1) = remainLen;
	} else {
	    // first byte is remainder (mod) of 128, then set the MSB to indicate more bytes
	    *(fixed_header+1) = remainLen % 128;
	    *(fixed_header+1) = *(fixed_header+1) | 0x80;
	    // second byte is number of 128s
	    *(fixed_header+2) = remainLen / 128;
	}

	packet_length = fixedHeaderSize+sizeof(var_header)+payload_len;/*固定报头+可变报头+负载长度*/
	packet = (uint8 *)malloc(packet_length);/*分配内存*/
	memset(packet, 0, packet_length);
	memcpy(packet, fixed_header, fixedHeaderSize);/*填充固定报头*/
	free(fixed_header);	
	offset += fixedHeaderSize;
	memcpy(packet+offset, var_header, sizeof(var_header));/*填充可变报头*/
	offset += sizeof(var_header);
	// Client ID - UTF encoded,填充clientid长度+clientid
	packet[offset++] = clientidlen>>8;
	packet[offset++] = clientidlen&0xFF;
	memcpy(packet+offset, client_id, clientidlen);
	offset += clientidlen;

	if(usernamelen) {
		// Username - UTF encoded,填充用户名+用户名长度
		packet[offset++] = usernamelen>>8;
		packet[offset++] = usernamelen&0xFF;
		memcpy(packet+offset, user_name, usernamelen);
		offset += usernamelen;
	}

	if(passwordlen) {
		// Password - UTF encoded,填充用户密码+用户名密码长度
		packet[offset++] = passwordlen>>8;
		packet[offset++] = passwordlen&0xFF;
		memcpy(packet+offset, passwd, passwordlen);
		offset += passwordlen;
	}
	// Send the packet
    if (client_send(sockfd,packet, packet_length) < 0){
         free(packet);
        return -1;
    }
    free(packet);
	return 1;
}

int mqtt_disconnect(int sockfd) 
{
	uint8 packet[] = {
		MQTT_MSG_DISCONNECT, // Message Type, DUP flag, QoS level, Retain
		0x00 // Remaining length
	};
	int ret = client_send(sockfd,packet, sizeof(packet));
    DEBUG_INFO("ret=%d",ret);
	return ret;
}

int mqtt_ping(int sockfd)
{
	uint8 packet[] = {
		MQTT_MSG_PINGREQ, // Message Type, DUP flag, QoS level, Retain
		0x00 // Remaining length
	};
	int ret = client_send(sockfd,packet, sizeof(packet));
    //DEBUG_INFO("ret=%d",ret);
	return ret;
}

static uint16 su_seq = 1;
int mqtt_subscribe_theme(int sockfd,char *Theme , uint8_t Qos)
{
	uint16_t MessageId = su_seq++;
	uint8_t cmd[1024]={0};
    //报文标示符长度2 + 主题长度位占用2字节+主题长度+qos标识
	int data_length = 2+2+strlen(Theme)+1; 
	int playload_len = strlen(Theme);
	uint8_t len_byte[4] ={0x00 , 0x00 ,0x00 ,0x00};
	uint8_t byte_num = length_trans_byte_form(data_length , len_byte);
	cmd[0] = 0x82;
	memcpy(&cmd[1] , len_byte , byte_num);

	cmd[1+byte_num]=(MessageId & 0xff00) >> 8 ;
	cmd[1+byte_num+1] = MessageId & 0x00ff;		
	cmd[1+byte_num+1+1] = (playload_len & 0xff00) >> 8;
	cmd[1+byte_num+1+1+1] = playload_len & 0x00ff;
	memcpy(&cmd[1+byte_num+1+1+1+1] , Theme , playload_len);
	cmd[1+byte_num+1+1+1+1+playload_len] = Qos;
	client_send(sockfd,cmd, 1+byte_num+1+1+1+1+playload_len+1);
}


static uint16 un_seq = 1;
int mqtt_unsubscribe_theme(int sockfd,const char* topic)
{
    uint16_t MessageId = un_seq++;
	uint8_t cmd[1024]={0};
    //报文标示符长度2 + 主题长度位占用2字节+主题长度+qos标识
	int data_length = 2+2+strlen(topic)+1; 
	int playload_len = strlen(topic);
	uint8_t len_byte[4] ={0x00 , 0x00 ,0x00 ,0x00};
	uint8_t byte_num = length_trans_byte_form(data_length , len_byte);
	cmd[0] = 0xa2;
	memcpy(&cmd[1] , len_byte , byte_num);
    
	cmd[1+byte_num]=(MessageId & 0xff00) >> 8 ;
	cmd[1+byte_num+1] = MessageId & 0x00ff;		
	cmd[1+byte_num+1+1] = (playload_len & 0xff00) >> 8;
	cmd[1+byte_num+1+1+1] = playload_len & 0x00ff;
	memcpy(&cmd[1+byte_num+1+1+1+1] , topic , playload_len);
	client_send(sockfd,cmd,1+byte_num+1+1+1+1+playload_len+1);
	return 1;
}

uint8_t length_trans_byte_form(int len , uint8_t * len_byte)
{
	uint8_t byte_num=0;
	uint8_t encode_byte =0;
	do{
		encode_byte = len % 128;
		len = len / 128 ;
		if(len > 0){
			encode_byte = encode_byte | 128;
		}
		len_byte[byte_num]=encode_byte;
		byte_num++;
	}while(len > 0);
	return byte_num;
}

//qos = 1和2时，用到
static uint16 publish_seq = 0;
int mqtt_publish(int clientfd,const char* topic, const char* msg, uint16 msglen, uint8 retain) 
{
	return mqtt_publish_with_qos(clientfd,topic, msg, msglen, retain, QOS_VALUE2, NULL);
}

int mqtt_publish_with_qos(int clientfd,const char* topic, const char* msg, uint16 msgl, uint8 retain, uint8 qos, uint16* message_id) 
{
    //DEBUG_INFO("qos:%d",qos);
	uint16 topiclen = strlen(topic);
	//uint16 msglen = strlen(msg);
	uint16 msglen = msgl;
	uint8 *var_header = NULL; // Topic size (2 bytes), utf-encoded topic
	uint8 *fixed_header = NULL;
	uint8 fixedHeaderSize = 0,var_headerSize = 0;    // Default size = one byte Message Type + one byte Remaining Length
	uint16 remainLen = 0;
	uint8 *packet = NULL;
	uint16 packet_length = 0;

	uint8 qos_flag = MQTT_QOS0_FLAG;
	uint8 qos_size = 0; // No QoS included
	if(qos == 1) {
		qos_size = 2; // 2 bytes for QoS
		qos_flag = MQTT_QOS1_FLAG;
	}
	else if(qos == 2) {
		qos_size = 2; // 2 bytes for QoS
		qos_flag = MQTT_QOS2_FLAG;
	}

	// Variable header
	var_headerSize = topiclen+2+qos_size;
	var_header = (uint8 *)malloc(var_headerSize);
	memset(var_header, 0, var_headerSize);
	*var_header = topiclen>>8;
	*(var_header+1) = topiclen&0xFF;
	memcpy(var_header+2, topic, topiclen);
	if(qos_size) {
        publish_seq++;
        if (publish_seq == 0){
            publish_seq = 1;
        }
		var_header[topiclen+2] = (publish_seq & 0xff00)>>8;
		var_header[topiclen+3] = publish_seq & 0x00ff;
		if(message_id) { 
			*message_id = publish_seq;
		}
	}

	// Fixed header
	// the remaining length is one byte for messages up to 127 bytes, then two bytes after that
	// actually, it can be up to 4 bytes but I'm making the assumption the embedded device will only
	// need up to two bytes of length (handles up to 16,383 (almost 16k) sized message)
	fixedHeaderSize = 2;    // Default size = one byte Message Type + one byte Remaining Length
	remainLen = var_headerSize+msglen;
	if (remainLen > 127) {
		fixedHeaderSize++;          // add an additional byte for Remaining Length
	}
	fixed_header = (uint8 *)malloc(fixedHeaderSize);
    
	// Message Type, DUP flag, QoS level, Retain
	*fixed_header = MQTT_MSG_PUBLISH | qos_flag;
	if(retain) {
		*fixed_header  |= MQTT_RETAIN_FLAG;
	}
	// Remaining Length
	if (remainLen <= 127) {
	   *(fixed_header+1) = remainLen;
	} else {
	   // first byte is remainder (mod) of 128, then set the MSB to indicate more bytes
	   *(fixed_header+1) = remainLen % 128;
	   *(fixed_header+1) = *(fixed_header+1) | 0x80;
	   // second byte is number of 128s
	   *(fixed_header+2) = remainLen / 128;
	}

	packet_length = fixedHeaderSize+var_headerSize+msglen;
	//packet = (uint8 *)malloc(packet_length);
	packet = (uint8 *)malloc(packet_length);
	memset(packet, 0, packet_length);
	memcpy(packet, fixed_header, fixedHeaderSize);
	memcpy(packet+fixedHeaderSize, var_header, var_headerSize);
	memcpy(packet+fixedHeaderSize+var_headerSize, msg, msglen);
	free(var_header);
	free(fixed_header);
    client_send(clientfd,packet , packet_length);
	free(packet);
	return 1;
}

