#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/signal.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <syslog.h>  
#include <sys/epoll.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>

#include "SocketServer.h"
#include "debug.h"
 
int main(void)
{
	pthread_t socketrevid;
	if (MqttSocketSeverInit(1883) == -1) {
		DEBUG_INFO("socket server start fail");
		exit(-1);
	}
    pthread_create(&socketrevid, NULL, &MqttSeverEpoll,NULL);
    while(1) {
        sleep(5);
    }
	return 0;
}
