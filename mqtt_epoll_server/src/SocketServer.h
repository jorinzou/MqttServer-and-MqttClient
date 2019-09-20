#ifndef SOCKET_SERVER_H
#define SOCKET_SERVER_H

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;

typedef struct {
	void *next;
	int SocketFd;
	socklen_t ClientAddrLen;
	struct sockaddr_in ClientAddr;
	uint8_t IsConnect;
	uint8_t ReadMoreNum ;
	uint8_t ReadMoreData[5];
} MqttSocketRecord_t;

#define MAX_CLIENTS 50
#define LISTEN 20
#define EVENT_SIZE LISTEN
#define EPOLL_SIZE EVENT_SIZE
#define MAXLINE 5

MqttSocketRecord_t *GetAuthBySockfd(int ClinetFd);
int MqttSocketSeverInit(int port);
void MqttGetClientFds(int *fds, int maxFds);
int MqttGetNumClients(void);
void MqttSeverPoll(int ClinetFd, int revent);
int MqttSeverSend(uint8_t* buf, int len, int ClientFd);
void CloseSocketFd(void);
void MqttPublishMessageDeal(int ClinetFd , char * RevJsonData);
void MqttSendPublishMessage(int ClinetFd , char * Playload, int PlayLoadLen, char*Theme ,uint16_t ThemeLen);
void DeleteSocketRecord(int RmSocketFd);
int CreateSocketRecord(void);
void AddEpollEvent(int SockFd);
void DelEpollEvent(int SockFd);
void SetLinger(const int sock, unsigned l_onoff);
void* MqttSeverEpoll(void* userData);
extern int epfd;

#endif
