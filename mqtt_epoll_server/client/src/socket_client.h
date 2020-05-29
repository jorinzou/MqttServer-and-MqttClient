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
#include "cmd.h"

extern int g_sockfd;

#define SERVER_PORT 1883
#define SERVER_IP "172.16.8.189"

int client_send(int fd, void *buf, size_t n) ;
int connect_server(int port ,const char *ipaddr,int sockfd);
int socket_init(void);
void *client_do_pthread(void *arg);
void *mqtt_ping_pthread(void *arg);
void mqtt_qos2_pubrel(int sockfd , unsigned char *data,unsigned char head_type);

#endif

