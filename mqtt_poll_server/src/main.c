#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <pthread.h>
#include <sys/signal.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <syslog.h>  
#include "SocketServer.h"
#include "debug.h"

#define POLLRDHUP 0x2000

void *SocketServerPoll(void *arg)
{
    pthread_detach(pthread_self());
	int pollFdIdx;
	while (1) {
		int numClientFds = MqttGetNumClients();
		if (numClientFds > 0) {
			int *client_fds = malloc(numClientFds * sizeof(int));
			struct pollfd *pollFds = malloc((numClientFds) * sizeof(struct pollfd));
			if (client_fds && pollFds) {
				MqttGetClientFds(client_fds,numClientFds);
				for (pollFdIdx = 0;pollFdIdx < numClientFds;pollFdIdx++) {
					pollFds[pollFdIdx].fd = client_fds[pollFdIdx];
					pollFds[pollFdIdx].events = POLLIN | POLLRDHUP;
				}
				poll (pollFds , numClientFds , -1);		
				for (pollFdIdx = 0;pollFdIdx < numClientFds; pollFdIdx++) {
					if (pollFds[pollFdIdx].revents) {
						MqttSeverPoll(pollFds[pollFdIdx].fd,pollFds[pollFdIdx].revents);
					}
				}
				free(client_fds);
				free(pollFds);			
			}
		}
	}	
	return  ;
}
 
int main(void)
{
	pthread_t socketrevid;
	if (MqttSocketSeverInit(1883) == -1) {
		DEBUG_INFO("socket server start fail");
		exit(-1);
	}
	pthread_create(&socketrevid, NULL, &SocketServerPoll,NULL);
    while(1) {
        sleep(5);
    }
	return 0;
}
