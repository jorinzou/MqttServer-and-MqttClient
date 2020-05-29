#ifndef _MQTT_H_
#define _MQTT_H_


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "debug.h"

typedef unsigned char  uint8;
typedef unsigned short uint16;


#define DEBUG_LEVEL 2

#ifndef MQTT_CONF_USERNAME_LENGTH
	#define MQTT_CONF_USERNAME_LENGTH 1024 // Recommended by MQTT Specification (12 + '\0')
#endif

#ifndef MQTT_CONF_PASSWORD_LENGTH
	#define MQTT_CONF_PASSWORD_LENGTH 1024 // Recommended by MQTT Specification (12 + '\0')
#endif

#define MQTT_MSG_CONNECT       1<<4
#define MQTT_MSG_CONNACK       2<<4
#define MQTT_MSG_PUBLISH       3<<4
#define MQTT_MSG_PUBACK        4<<4
#define MQTT_MSG_PUBREC        5<<4
#define MQTT_MSG_PUBREL        6<<4
#define MQTT_MSG_PUBCOMP       7<<4
#define MQTT_MSG_SUBSCRIBE     8<<4
#define MQTT_MSG_SUBACK        9<<4
#define MQTT_MSG_UNSUBSCRIBE  10<<4
#define MQTT_MSG_UNSUBACK     11<<4
#define MQTT_MSG_PINGREQ      12<<4
#define MQTT_MSG_PINGRESP     13<<4
#define MQTT_MSG_DISCONNECT   14<<4

#define MSG_CONNECT       1
#define MSG_CONNACK       2
#define MSG_PUBLISH       3
#define MSG_PUBACK        4
#define MSG_PUBREC        5
#define MSG_PUBREL        6
#define MSG_PUBCOMP       7
#define MSG_SUBSCRIBE     8
#define MSG_SUBACK        9
#define MSG_UNSUBSCRIBE  10
#define MSG_UNSUBACK     11
#define MSG_PINGREQ      12
#define MSG_PINGRESP     13
#define MSG_DISCONNECT   14


/** Extract the message type from buffer.
 * @param buffer Pointer to the packet.
 *
 * @return Message Type byte.
 */
#define MQTTParseMessageType(buffer) ( *buffer & 0xF0 )

/** Indicate if it is a duplicate packet.
 * @param buffer Pointer to the packet.
 *
 * @retval   0 Not duplicate.
 * @retval !=0 Duplicate.
 */
#define MQTTParseMessageDuplicate(buffer) ( *buffer & 0x08 )

/** Extract the message QoS level.
 * @param buffer Pointer to the packet.
 *
 * @return QoS Level (0, 1 or 2).
 */
#define MQTTParseMessageQos(buffer) ( (*buffer & 0x06) >> 1 )

/** Indicate if this packet has a retain flag.
 * @param buffer Pointer to the packet.
 *
 * @retval   0 Not duplicate.
 * @retval !=0 Duplicate.
 */
#define MQTTParseMessageRetain(buffer) ( *buffer & 0x01 )


/** Parse packet buffer for number of bytes in remaining length field.
 *
 * Given a packet, return number of bytes in remaining length
 * field in MQTT fixed header.  Can be from 1 - 4 bytes depending
 * on length of message.
 *
 * @param buf Pointer to the packet.
 *
 * @retval number of bytes
 */
uint8 mqtt_num_rem_len_bytes(const uint8* buf);

/** Parse packet buffer for remaning length value.
 *
 * Given a packet, return remaining length value (in fixed header).
 * 
 * @param buf Pointer to the packet.
 *
 * @retval remaining length
 */
uint16 mqtt_parse_rem_len(const uint8* buf);

/** Parse packet buffer for message id.
 *
 * @param buf Pointer to the packet.
 *
 * @retval message id
 */
uint16 mqtt_parse_msg_id(const uint8* buf);

/** Parse a packet buffer for the publish topic.
 *
 * Given a packet containing an MQTT publish message,
 * return the message topic.
 *
 * @param buf Pointer to the packet.
 * @param topic  Pointer destination buffer for topic
 *
 * @retval size in bytes of topic (0 = no publish message in buffer)
 */
uint16 mqtt_parse_pub_topic(const uint8* buf, uint8* topic);

/** Parse a packet buffer for a pointer to the publish topic.
 *
 *  Not called directly - called by mqtt_parse_pub_topic
 */
uint16 mqtt_parse_pub_topic_ptr(const uint8* buf, const uint8** topic_ptr);

/** Parse a packet buffer for the publish message.
 *
 * Given a packet containing an MQTT publish message,
 * return the message.
 *
 * @param buf Pointer to the packet.
 * @param msg Pointer destination buffer for message
 *
 * @retval size in bytes of topic (0 = no publish message in buffer)
 */
uint16 mqtt_parse_publish_msg(const uint8* buf, uint8* msg);

/** Parse a packet buffer for a pointer to the publish message.
 *
 *  Not called directly - called by mqtt_parse_pub_msg
 */
uint16 mqtt_parse_pub_msg_ptr(const uint8* buf, const uint8** msg_ptr);

int mqtt_connect(int sockfd);
int mqtt_disconnect(int sockfd) ;
int mqtt_ping(int sockfd);
int mqtt_subscribe_theme(int sockfd,char *Theme , uint8_t Qos);
int mqtt_unsubscribe_theme(int sockfd,const char* topic);
uint8_t length_trans_byte_form(int Length , uint8_t * len_byte);
int mqtt_publish(int clientfd,const char* topic, const char* msg, uint16 msglen, uint8 retain);
int mqtt_publish_with_qos(int clientfd,const char* topic, const char* msg, uint16 msgl, uint8 retain, uint8 qos, uint16* message_id);

#endif

