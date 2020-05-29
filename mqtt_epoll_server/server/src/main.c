#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "pthread.h"
#include "socket.h"

int main_loop(void)
{
    int ret = socket_init(SERVER_PORT);
    if (ret < 0){
        DEBUG_INFO("socket_init failed");
        return -1;
    }
    pthread_t t1;
    pthread_create(&t1,NULL,server_do_pthread,NULL);


    while(1){
        sleep(100);
    }
	return 0;
}

int main(void)
{
//#define DEFINE_DAEMON

#ifdef DEFINE_DAEMON
     pid_t pid = fork();
    switch(pid){
        case 0://成为守护进程
            main_loop();
        break;

        default:
            exit(1);
        break;
    }
#else
    main_loop();
#endif
    return 0;
}
