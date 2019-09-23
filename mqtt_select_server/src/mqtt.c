#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>
#include <sys/time.h>
#include <syslog.h>
#include "cJSON.h"
#include "SocketServer.h"
#include "cJSON.h"
#include "mqtt.h"
#include "debug.h"

/*计算剩余报文长度*/
int CalculateRemainLength(uint8_t * Data)
{
	int multiplier =1;
	int Lenght = 0 ;
	uint8_t encodedByte;
	do {
		Data++;
		encodedByte = *Data;
		Lenght+=(encodedByte & 127)*multiplier;
		multiplier*=128;
		if (multiplier > 128*128*128) {
			return -1;
		}
	} while((encodedByte & 128)!=0);
	return Lenght;
}

/*剩余报文转编码*/
uint8_t LengthTransformByte(int Length , uint8_t *LengthByte)
{
	uint8_t ByteNum=0;
	uint8_t EncodedByte =0;
	do {
		EncodedByte = Length % 128;
		Length = Length / 128 ;
		if(Length > 0)
			EncodedByte = EncodedByte | 128;
		LengthByte[ByteNum]=EncodedByte;
		ByteNum++;
	} while(Length > 0);
	return ByteNum;
}

void MqttConnect( int ClinetFd , uint8_t * Data)
{
	uint8_t cmd[]={ 0x20, 0x02 ,0x00,0x00};
	MqttSeverSend(cmd,sizeof(cmd),ClinetFd);
	MqttSocketRecord_t *SocketRecord=GetAuthBySockfd(ClinetFd);
	if (SocketRecord==NULL) {
		return;
	}
	SocketRecord->IsConnect=0x01;	
}

void MqttPublish(int ClinetFd , uint8_t *Data ,uint8_t HeadLow4Bit)
{
	int DataLength = CalculateRemainLength(Data);
	if (DataLength < 0) {
		return;
	}
    
	int LengthByteNum=0;
	if (DataLength < 128) {
		LengthByteNum=1;
	}
    else if (DataLength < 16384) {
		LengthByteNum=2;
	}
    else if(DataLength < 2097152) {
		LengthByteNum=3;
	}
    else if(DataLength < 268435456) {
		LengthByteNum=4;
	}

	int Pos=LengthByteNum+1;
	uint16_t ThemeNameLength = (( Data[Pos] & 0x00ff )<< 8) | (Data[Pos+1] & 0x00ff);
	Pos+=2;
	if (ThemeNameLength > 1024) {
		return;
	}
	char *ThemeName = (char *)malloc(ThemeNameLength+1);
	memset(ThemeName , 0 ,ThemeNameLength+1);
	memcpy(ThemeName ,&Data[Pos] , ThemeNameLength);
	DEBUG_INFO("ThemeName:%s",ThemeName);
	Pos+=ThemeNameLength;
	uint16_t PackageId = 0;
	uint8_t PackageIdLength=0;
	uint8_t Qos=( HeadLow4Bit & 0x06 ) >> 1;
	if (Qos  > 0 && Qos < 3) {
		DEBUG_INFO("Qos : %d " ,Qos);
		PackageId = ((Data[Pos] & 0x00ff) <<8) | (Data[Pos+1] & 0x00ff);
		Pos+=2;
		PackageIdLength=2;
		if (Qos==1) {
			uint8_t Qos1_Respon[]={ 0x40 ,0x02 , (PackageId & 0xff00) >> 8, PackageId & 0x00ff };
			MqttSeverSend(Qos1_Respon,sizeof(Qos1_Respon),ClinetFd);
		}
		if (Qos == 2) {
			uint8_t Qos2_Respon[]={0x50 ,0x02 , (PackageId & 0xff00) >> 8, PackageId & 0x00ff };
			MqttSeverSend(Qos2_Respon,sizeof(Qos2_Respon),ClinetFd);
		}
	}
	int playload_length = DataLength - ThemeNameLength - 2 -PackageIdLength;
	if (playload_length > 8192 || playload_length < 0) {
		free(ThemeName);
		return;
	}
    char Playload[8192]={0};
    memcpy(Playload , &Data[Pos],playload_length);
    MqttPublishMessageDeal(ClinetFd ,Playload);
    free(ThemeName);
}

void MqttQos2Pubrel(int ClinetFd , uint8_t *Data)
{
	uint8_t QOS2Pubrel_Respon[]={ 0x70 ,0x02 , Data[2] , Data[3] };
	MqttSeverSend(QOS2Pubrel_Respon,sizeof(QOS2Pubrel_Respon),ClinetFd);
}

void MqttSubscribe(int ClinetFd , uint8_t *Data)
{
	int DataLength=CalculateRemainLength(Data);
	if(DataLength < 0)
		return;
	int LengthByteNum=0;
	if (DataLength < 128) {
	
		LengthByteNum=1;
	}
    else if(DataLength < 16384) {
		LengthByteNum=2;
	}
    else if(DataLength < 2097152) {
		LengthByteNum=3;
	}
    else if(DataLength < 268435456) {
		LengthByteNum=4;
	}
	if (DataLength > 1024)
		return;
    
	int Pos=LengthByteNum+1;
	uint16_t PackageId= ((Data[Pos] & 0x00ff) <<8) | (Data[Pos+1] & 0x00ff);
	Pos+=2;
	int PlayLoadLen = DataLength - 2;
	do {
		uint16_t Topic_length=((Data[Pos] & 0x00ff) <<8) | (Data[Pos+1] & 0x00ff);
		Pos+=2;
		if(Pos+Topic_length > DataLength+1)
			break;
		char Topic[1025];
		memset(Topic , 0 , sizeof(Topic));
		memcpy(Topic , &Data[Pos] ,Topic_length);
		DEBUG_INFO("Topic: %s " , Topic);
		Pos+=(Topic_length+1);
	}
	while(Pos < DataLength+1);
	uint8_t Subscribe_Respon[]= { 0x90  ,0x03 , (PackageId & 0xff00 )>> 8 , PackageId & 0x00ff , 0x00 };
    MqttSeverSend(Subscribe_Respon, sizeof(Subscribe_Respon),ClinetFd);
}


void MqttUnSubscribe(int ClinetFd , uint8_t *Data)
{
	int DataLength=CalculateRemainLength(Data);
	if(DataLength < 0)
		return;
	int LengthByteNum=0;
	if (DataLength < 128) {
		LengthByteNum=1;
	}
    else if(DataLength < 16384) {
		LengthByteNum=2;
	}
    else if(DataLength < 2097152) {
		LengthByteNum=3;
	}
    else if(DataLength < 268435456) {
		LengthByteNum=4;
	}
	if(DataLength > 1025)
		return;
    
	int Pos=LengthByteNum+1;
	uint16_t PackageId= ((Data[Pos] & 0x00ff) <<8) | (Data[Pos+1] & 0x00ff);
	Pos+=2;
	int PlayLoadLen = DataLength - 2;
	do {
		uint16_t Topic_length=((Data[Pos] & 0x00ff) <<8) | (Data[Pos+1] & 0x00ff);
		Pos+=2;
		if(Pos+Topic_length > DataLength+1)
			break;
		char Topic[1025];
		memset(Topic , 0 , sizeof(Topic));
		memcpy(Topic , &Data[Pos] ,Topic_length);
		DEBUG_INFO("Topic: %s " , Topic);
		Pos+=(Topic_length);
	}
	while (Pos < DataLength+1);
	uint8_t UnSubscribe_Respon[]={
		0xb0  ,0x02 , (PackageId & 0xff00 )>> 8 , PackageId & 0x00ff
	};
	
	MqttSeverSend(UnSubscribe_Respon, sizeof(UnSubscribe_Respon),ClinetFd);
}

void MqttPingReply(int ClinetFd)
{
	uint8_t PingResp[]={ 0xd0 , 0x00 };
	MqttSeverSend(PingResp, sizeof(PingResp),ClinetFd);
}

