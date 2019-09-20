#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/signal.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include <syslog.h>

#include "cJSON.h"
#include "mqtt.h"
#include "SocketServer.h"
#include "debug.h"

#define POLLRDHUP	0x2000

MqttSocketRecord_t *MqttsocketRecordHead = NULL;

int CreateSocketRecord(void)
{
	int tr = 1;
	MqttSocketRecord_t *SrcRecord;
	MqttSocketRecord_t *NewSocket = calloc(1 , sizeof(MqttSocketRecord_t));
	NewSocket->ClientAddrLen = sizeof(NewSocket->ClientAddr);
	NewSocket->SocketFd = accept(MqttsocketRecordHead->SocketFd,(struct sockaddr *) &(NewSocket->ClientAddr), &(NewSocket->ClientAddrLen));
	if (NewSocket->SocketFd < 0) {
		DEBUG_INFO("ERROR on accept");
		free(NewSocket);
		return -1;
	}

	// "Address Already in Use" error on the bind
	setsockopt(NewSocket->SocketFd, SOL_SOCKET, SO_REUSEADDR, &tr, sizeof(int));
	fcntl(NewSocket->SocketFd, F_SETFL, O_NONBLOCK);
	NewSocket->next = NULL;
	SrcRecord = MqttsocketRecordHead;
	while (SrcRecord->next)
		SrcRecord = SrcRecord->next;

	SrcRecord->next = NewSocket;
	return (NewSocket->SocketFd);
}

void DeleteSocketRecord(int RmSocketFd)
{
	MqttSocketRecord_t *SrcRecord, *PrevRecord = NULL;
	SrcRecord = MqttsocketRecordHead;
	while ((SrcRecord->SocketFd != RmSocketFd) && (SrcRecord->next)) {
		PrevRecord = SrcRecord;
		SrcRecord = SrcRecord->next;
	}

	if (SrcRecord->SocketFd != RmSocketFd) {
		return;
	}

	if (SrcRecord) {
		if (PrevRecord == NULL) {
			return;
		}
		PrevRecord->next = SrcRecord->next;
		close(SrcRecord->SocketFd);
		free(SrcRecord);
	}
}

int MqttSocketSeverInit(int port)
{
	struct sockaddr_in serv_addr;
	int stat, tr = 1;

	if (MqttsocketRecordHead == NULL) {	
		MqttSocketRecord_t *lsSocket = malloc(sizeof(MqttSocketRecord_t));
		lsSocket->SocketFd = socket(AF_INET, SOCK_STREAM, 0);
		if (lsSocket->SocketFd < 0) {
			DEBUG_INFO("ERROR opening socket");
			return -1;
		}

		setsockopt(lsSocket->SocketFd, SOL_SOCKET, SO_REUSEADDR, &tr, sizeof(int));
		// Set the fd to none blocking
		fcntl(lsSocket->SocketFd, F_SETFL, O_NONBLOCK);
		bzero((char *) &serv_addr, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = INADDR_ANY;
		serv_addr.sin_port = htons(port);
		stat = bind(lsSocket->SocketFd, (struct sockaddr*) &serv_addr,sizeof(serv_addr));
		if (stat < 0) {
			DEBUG_INFO("ERROR on binding: %s\n", strerror(errno));
			return -1;
		}
		listen(lsSocket->SocketFd, 5);
		lsSocket->next = NULL;
		MqttsocketRecordHead = lsSocket;
	}

	return 0;
}

void MqttGetClientFds(int *fds, int maxFds)
{
	int recordCnt = 0;
	MqttSocketRecord_t *SrcRecord;
	SrcRecord = MqttsocketRecordHead;
	while ((SrcRecord) && (recordCnt < maxFds)) {
		fds[recordCnt++] = SrcRecord->SocketFd;
		SrcRecord = SrcRecord->next;
	}
	return;
}

int MqttGetNumClients(void)
{
	int recordCnt = 0;
	MqttSocketRecord_t *SrcRecord;
	SrcRecord = MqttsocketRecordHead;
	if (SrcRecord == NULL) {
		return -1;
	}

	while (SrcRecord) {
		SrcRecord = SrcRecord->next;
		recordCnt++;
	}
	return (recordCnt);
}

MqttSocketRecord_t * GetAuthBySockfd(int ClinetFd)
{
	MqttSocketRecord_t * SrcRecord;
	SrcRecord = MqttsocketRecordHead;
	while (SrcRecord!=NULL) {
		if(SrcRecord->SocketFd == ClinetFd)
			return SrcRecord;
		else
			SrcRecord=SrcRecord->next;
	}
	return NULL;
}

void MqttReceiveParse(int ClinetFd)
{
	MqttSocketRecord_t *SocketRecord=GetAuthBySockfd(ClinetFd);
	if (SocketRecord==NULL) {
	
		return;
	}
	uint8_t ReadMoreNum = SocketRecord->ReadMoreNum;
 	uint8_t *ReadMoreData = SocketRecord->ReadMoreData;		
	static uint8_t Data[8193];
	memset(Data , 0 ,sizeof(Data));
	memcpy(Data , ReadMoreData , ReadMoreNum );
	int ret = read(ClinetFd , &Data[ReadMoreNum] , 5 - ReadMoreNum );
    
	if (ret < 1 || ret > (5-ReadMoreNum)) {
		perror("Socket Error:");
		SocketRecord->ReadMoreNum = 0;
		if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
			return;
		}
		DeleteSocketRecord(ClinetFd);
		return;
	}
    
	int DataLength=CalculateRemainLength(Data);
	char Log[200];
	memset(Log , 0 , sizeof(Log));
	if (DataLength < 0) {
		SocketRecord->ReadMoreNum = 0;
		return;
	}
	int LengthByteNum=0;
    
	if (DataLength < 128) {
		LengthByteNum=1;
	}
    else if (DataLength < 16384) {
		LengthByteNum=2;
	}
    else if (DataLength < 2097152) {
		LengthByteNum=3;
	}
    else if (DataLength < 268435456) {
		LengthByteNum=4;
	}
	if (DataLength > sizeof(Data)-1) {
		SocketRecord->ReadMoreNum = 0;
		return;
	}

    //当前Data的数据总量
	int ret2 = ret+ReadMoreNum;
	int ReadNum2= DataLength-(ret2- (LengthByteNum+1));
    //意外读到了下一条mqtt数据(粘包)
	if (ReadNum2 < 0 ) {
		ReadMoreNum = -ReadNum2;
		memcpy(ReadMoreData , &Data[ret2-ReadMoreNum] , ReadMoreNum);
		int n;
		for (n= 0 ; n < ReadMoreNum ; n++) {
		
			printf("%02x " , ReadMoreData[n]);
		}
		printf("\n");
		SocketRecord->ReadMoreNum =ReadMoreNum;
	}
    else {
		SocketRecord->ReadMoreNum = 0;
	}
    
	int retryAttempts=0;
	while (ReadNum2 > 0) {
		int byteRead =  read(ClinetFd , &Data[ret2] ,ReadNum2);
		if (byteRead == 0) {
			DeleteSocketRecord(ClinetFd);
			return;
		}
		
		if (byteRead < 0) {
			perror("Socket Error");
			if (retryAttempts++ < 1000) {
				usleep(1000);
				byteRead=0;					
			}
			else {
				DeleteSocketRecord(ClinetFd);
				return;
			}
		}
		ReadNum2 -= byteRead;
		ret2 += byteRead;
	}
    
	if (ret2 > 0) {
		char *ReviceData = (char *)calloc(1 , 2* ret2 +1);
		int n=0;
		for (n=0;n<ret2;n++) {
			sprintf(&ReviceData[2*n] ,"%02x" , Data[n]);
		}
		free(ReviceData);
	}
    else {
		DEBUG_INFO( "Revice Data Num < 0 , return");
		return;
	}
    
	uint8_t HeadHigh4Bit= (Data[0] & 0xf0) >> 4 ;
	uint8_t HeadLow4Bit= Data[0] & 0x0f;
	DEBUG_INFO("HeadHigh4Bit : %d , HeadLow4Bit : %d " , HeadHigh4Bit , HeadLow4Bit);
	if (SocketRecord->IsConnect!=0x01 && HeadHigh4Bit!=1) {
		DeleteSocketRecord(ClinetFd);
		return;
	}
    
	switch (HeadHigh4Bit) {
		case 1:
			DEBUG_INFO("message is CONNECT");
			if (HeadLow4Bit!=0) {
				DEBUG_INFO("HeadLow4Bit error");
				break;
			}
			MqttConnect(ClinetFd , Data);
		break;

		case 3:
			DEBUG_INFO("message is Publish");
			MqttPublish(ClinetFd , Data ,HeadLow4Bit);
		break;

		case 6:
			DEBUG_INFO("message is Qos PUBREL");
			if(HeadLow4Bit!=2) {
				DEBUG_INFO("HeadLow4Bit error");
				break;
			}
			MqttQos2Pubrel(ClinetFd, Data);
		break;

		case 8:
			DEBUG_INFO("message is Subscribe");
			if (HeadLow4Bit!=2) {
				DEBUG_INFO("HeadLow4Bit error");
				break;
			}
			MqttSubscribe(ClinetFd, Data);
		break;

		case 10:
			DEBUG_INFO("message is Unsubscribe");
			if (HeadLow4Bit!=2) {
				DEBUG_INFO("HeadLow4Bit error");
				break;
			}
			MqttUnSubscribe(ClinetFd, Data);
		break;

		case 12:
			DEBUG_INFO("Message is PingREQ");
			MqttPingReply(ClinetFd);
		break;

		case 14:
			DEBUG_INFO("Message is Disconnect");
			DeleteSocketRecord(ClinetFd);
		break;
		
		default:
			DEBUG_INFO("default\n");
		break;
	}
}

void MqttPublishMessageDeal(int ClinetFd , char * RevJsonData)
{
	cJSON *Json = cJSON_Parse(RevJsonData);
    char *StrJson = cJSON_Print(Json);
    DEBUG_INFO("%s",StrJson);

    MqttSocketRecord_t *SocketRecord = GetAuthBySockfd(ClinetFd);
    if (NULL == SocketRecord) {
        DEBUG_INFO("no exist this clientfd");
    }

    char theme[128] = {0};
    sprintf(theme,"/%s/%s",inet_ntoa((SocketRecord->ClientAddr).sin_addr),"MqttServerRespon");
    MqttSendPublishMessage(ClinetFd, StrJson, strlen(StrJson), theme, strlen(StrJson));
    free(StrJson);
	cJSON_Delete(Json);	
	return;
}

void MqttSeverPoll(int ClinetFd, int revent)
{
	if (ClinetFd == MqttsocketRecordHead->SocketFd) {
		CreateSocketRecord();
	}
	else {
		if (revent & POLLIN) {
			MqttReceiveParse(ClinetFd);

		}
		if (revent & POLLRDHUP) {
			DeleteSocketRecord(ClinetFd);
		}
  }
  return;
}

int MqttSeverSend(uint8_t* buf, int len, int ClientFd)
{
	int rtn;
	if (ClientFd) {
		int SendNum = 0;
		int SendLenth = len ;
		while(len) {
			rtn = write(ClientFd, &buf[SendLenth - len], len);
			if (rtn <=0) {
				perror("MqttSeverSend error ");
                /* EAGAIN : Resource temporarily unavailable*/
				if(errno == EINTR || errno == EAGAIN ) { 
					if(SendNum++ > 150) {
						printf("Resource temporarily unavailable\n");
						break;
					}
					usleep(10*1000);
				}
				else {
					printf("Write error, close socket\n");
					DeleteSocketRecord(ClientFd);
					break;
				}
				rtn = 0;
			}
			len-=rtn;
		}
		return rtn;
	}
	return 0;
}

void CloseSocketFd(void)
{
	int fds[MAX_CLIENTS], idx = 0;
	MqttGetClientFds(fds, MAX_CLIENTS);
    
	while (MqttGetNumClients() > 1) {
		DeleteSocketRecord(fds[idx++]);
	}
	if (fds[0]) {
		close(fds[0]);
	}
}

void MqttSendPublishMessage(int ClinetFd , char *Playload, int PlayLoadLen, char* Theme ,uint16_t ThemeLen)
{
    uint8_t LengthByte[4]={0x00 , 0x00 ,0x00 ,0x00};
	int DataLength = PlayLoadLen+ ThemeLen + 2 ;
	uint8_t ByteNum = LengthTransformByte(DataLength , LengthByte);
	uint8_t *SendBuff = (uint8_t*)malloc((DataLength+ByteNum+1 + 1)*sizeof(uint8_t) );
	memset(SendBuff , 0 , (DataLength+ByteNum+1 + 1)*sizeof(uint8_t) );
	SendBuff[0]= 0x30 ;
	memcpy(&SendBuff[1] , LengthByte ,ByteNum);
	SendBuff[1+ByteNum]=(ThemeLen & 0xff00) >> 8 ;
	SendBuff[1+ByteNum+1]=ThemeLen & 0x00ff;
	memcpy(&SendBuff[1+ByteNum+2] , Theme , ThemeLen);
	memcpy(&SendBuff[1+ByteNum+2+ThemeLen] , Playload , PlayLoadLen);
	MqttSeverSend(SendBuff , DataLength+ByteNum+1 , ClinetFd);
	free(SendBuff);	
}
