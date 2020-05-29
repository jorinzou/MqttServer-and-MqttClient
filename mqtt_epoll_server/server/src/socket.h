#ifndef _SOCKET_CLIENT_H
#define _SOCKET_CLIENT_H
#include <stdint.h>
#include <getopt.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <pthread.h>

#include "debug.h"
#include "mqtt.h"
#include "cJSON.h"
#include "epoll.h"

#define SERVER_PORT 1883

typedef struct {
	uint8_t read_more_num ;
    uint8_t is_connect;
	uint8_t read_more_data[5];
    uint32 publish_seq;
    void *next;
	int sockfd;
    int client_addr_len;
	struct sockaddr_in client_addr;
} socket_record_t;

int send_msg(int fd, void *buf, size_t n) ;
int socket_init(int port);
void *client_do_pthread(void *arg);
void *mqtt_ping_pthread(void *arg);
void mqtt_qos2_pubrel(int sockfd , unsigned char *data,unsigned char head_type);
void *server_do_pthread(void *arg);
socket_record_t * look_up_by_sokfd(int sockfd);

#endif

