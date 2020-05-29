#include <pthread.h>
#include <string.h>
#include <stdio.h>

#include "socket_client.h"
#include "mqtt.h"
#include "cmd.h"

void show_help(void)
{
    //system("clear");
    printf("//////////////////////////////////////////////\n");
    //printf("1 connect mqtt server\n");
    printf("2 disconnect mqtt server and exit client process\n");
    printf("3 ping mqtt server\n");
    printf("4 subscribe theme\n");
    printf("5 unsubscribe theme\n");
    printf("6 publish msg\n");
    printf("//////////////////////////////////////////////\n");
}

void *cmd_do_pthread(void *arg)
{
    int cmd = 0;
    while(1){
       show_help();
       scanf("%d",&cmd); 
       if (cmd == 1){
           //mqtt_connect_init(g_sockfd); 
       }
       else if (cmd == 2){
            mqtt_disconnect(g_sockfd);
            exit(0);
       }
       else if (cmd == 3){
            mqtt_ping(g_sockfd);
       }
       else if (cmd == 4){
            mqtt_subscribe_theme(g_sockfd,"client subscribe theme",2);
       }
       else if (cmd == 5){
            mqtt_unsubscribe_theme(g_sockfd,"client unsubscribe theme");
       }
       else if (cmd == 6){
            char msg[64] = {"client publish msg"};
            mqtt_publish(g_sockfd,"client publish theme",msg,strlen(msg),0);
       }
    }
}



