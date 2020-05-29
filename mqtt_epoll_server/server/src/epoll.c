#include <stdlib.h>

#include "epoll.h"
#include "debug.h"

int epfd = -1;

int mqtt_epoll_create(int flags) 
{
    int fd = epoll_create1(flags);
    if (fd < 0){
        perror("epoll_create1");
        DEBUG_INFO("epoll_create1");
    }
    return fd;
}

/*events可以是以下几个宏的集合：
EPOLLIN ：表示对应的文件描述符可以读（包括对端SOCKET正常关闭）；
EPOLLOUT：表示对应的文件描述符可以写；
EPOLLPRI：表示对应的文件描述符有紧急的数据可读（这里应该表示有带外数据到来）；
EPOLLERR：表示对应的文件描述符发生错误；
EPOLLHUP：表示对应的文件描述符被挂断；
EPOLLET： 将EPOLL设为边缘触发(Edge Triggered)模式，这是相对于水平触发(Level Triggered)来说的。
EPOLLONESHOT：只监听一次事件，当监听完这次事件之后，
如果还需要继续监听这个socket的话，需要再次把这个socket加入到EPOLL队列里 */
void mqtt_epoll_add(int epfd, int fd) 
{
    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events=EPOLLIN|EPOLLHUP;
    int rc = epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
    if (rc < 0){
        perror("epoll_ctl");
        DEBUG_INFO("epoll_ctl");
    }
    return;
}

void mqtt_epoll_mod(int epfd, int fd)
{
    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events=EPOLLIN|EPOLLHUP;
    int rc = epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
    if (rc < 0){
        perror("epoll_ctl");
        DEBUG_INFO("epoll_ctl");
    }
    return;
}

void mqtt_epoll_del(int epfd, int fd) 
{
    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events=EPOLLIN|EPOLLHUP;
    int rc = epoll_ctl(epfd, EPOLL_CTL_DEL, fd, &ev);
    if (rc < 0){
        perror("epoll_ctl");
        DEBUG_INFO("epoll_ctl");
    }
    return;
}

int mqtt_epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout)
{
    int n = epoll_wait(epfd, events, maxevents, timeout);
    if (n < 0){
        perror("epoll_wait");
        DEBUG_INFO("epoll_wait");
    }
    return n;
}

/*
设置 l_onoff 为非0，l_linger为非0，当套接口关闭时内核将拖延一段时间（由l_linger决定）如果套接口缓冲区中仍残留数据，进程将处于睡眠状态，
直到有数据发送完且被对方确认，之后进行正常的终止序列。
此种情况下,应用程序查close的返回是非常重要的，如果在数据发送完并被确认前时间到
close将返回EWOULDBLOCK错误且套接口发缓冲区中的任何数据都丢失
close的成功返回仅告诉我们发的数据（和FIN）已由对方TCP确认
它并不能告诉我们对方应用进程是否已读了数据,
如果套接口设为非阻塞的，它将不等待close完成
*/
void set_linger(const int sockfd, unsigned l_onoff)
{
	int z; /* Status code*/	
	struct linger so_linger;
	so_linger.l_onoff = l_onoff;
	so_linger.l_linger = 30;
	z = setsockopt(sockfd, SOL_SOCKET, SO_LINGER, &so_linger, sizeof( so_linger));
	if (z) {
		perror("setsockopt(2)");
	}
}

