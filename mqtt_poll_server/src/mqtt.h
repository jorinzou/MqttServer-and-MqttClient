#ifndef _MQTT_H_
#define _MQTT_H_
#include "SocketServer.h"

int CalculateRemainLength(uint8_t * Data);
void MqttConnect( int ClinetFd , uint8_t * Data);
void MqttPublish(int ClinetFd , uint8_t *Data ,uint8_t HeadLow4Bit);
void MqttSubscribe(int ClinetFd , uint8_t *Data);
void MqttPingReply(int ClinetFd);
void MqttUnSubscribe(int ClinetFd , uint8_t *Data);
void MqttQos2Pubrel(int ClinetFd , uint8_t *Data);

#endif
