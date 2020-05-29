#ifndef _EPOLL_H_
#define _EPOLL_H_
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
#include <sys/epoll.h>

#define MAXEVENTS 20
extern int epfd;
int mqtt_epoll_create(int flags);
void mqtt_epoll_add(int epfd, int fd);
void mqtt_epoll_mod(int epfd, int fd);
void mqtt_epoll_del(int epfd, int fd);
int mqtt_epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);
void set_linger(const int sockfd, unsigned l_onoff);
#endif
