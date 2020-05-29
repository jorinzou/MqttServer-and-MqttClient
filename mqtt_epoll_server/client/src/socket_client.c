#include "socket_client.h"

int g_sockfd = -1;

int socket_init(void)
{
    int sockfd = -1;
    int optval=1;
    struct sockaddr_in serveraddr;
  
    /* Create a socket descriptor */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("socket");
	    return -1;
    }
 
    /* Eliminates "Address already in use" error from bind. */
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,(const void *)&optval , sizeof(int)) < 0){
        perror("setsockopt");
	    return -1;
    }
    return sockfd;
} 

int connect_server(int port ,const char *ipaddr,int sockfd)
{
	struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
	server_addr.sin_addr.s_addr=inet_addr(ipaddr);
    
    int ret = connect(sockfd,(struct sockaddr*)&server_addr,sizeof(struct sockaddr));
    if (ret < 0){
        perror("connect");
        DEBUG_INFO("connect server failed");
        return -1;
    }
     /*设置成非阻塞*/
    unsigned long ul = 1;
    ioctl(g_sockfd, FIONBIO, &ul); 
    DEBUG_INFO("connect server sucessful");
    return 0;
}
int client_send(int fd, void *buf, size_t n) 
{
    size_t nleft = n;
    int nwritten;
    char *bufp = (char *)buf;
        
    while (nleft > 0) {
        if ((nwritten = write(fd, bufp, nleft)) <= 0) {
            if (errno == EINTR || errno == EAGAIN)  /* interrupted by sig handler return */
                nwritten = 0;    /* and call write() again */
            else {
                DEBUG_INFO("%s", strerror(errno));
                perror("write");
                return -1;       /* errorno set by write() */
            }
        }
        nleft -= nwritten;
        bufp += nwritten;
    }
    return n;
}

void do_topic_and_msg(const uint8* topic,const uint8* msg)
{
    //do something
    DEBUG_INFO("topic=%s",topic);
    //DEBUG_INFO("msg=%s",msg);
}

void mqtt_do_publish_msg(const uint8 *buf,int sockfd)
{
    //do qos
    uint8 qos = MQTTParseMessageQos(buf);
    //DEBUG_INFO("qos=%d");

    if(qos == 0){
        //do nothing;
    }
    else if(qos == 1){
        //messge id
        uint16 msg_id = mqtt_parse_msg_id(buf);
        uint8 qos1_respon[]={0x40 ,0x02 , (msg_id & 0xff00) >> 8, msg_id & 0x00ff};
	    client_send(sockfd,qos1_respon,sizeof(qos1_respon));
    }
    else if(qos == 2){
        //messge id
        uint16 msg_id = mqtt_parse_msg_id(buf);
        uint8 qos2_respon[]={0x50 ,0x02 , (msg_id & 0xff00) >> 8, msg_id & 0x00ff};
	    client_send(sockfd,qos2_respon,sizeof(qos2_respon));
    }
    else {
        DEBUG_INFO("unkonw qos=%d",qos);
        return;
    }
    
    //do topic
    const uint8* topic_ptr = NULL;
	uint16 topic_len = mqtt_parse_pub_topic_ptr(buf, &topic_ptr);
    if (topic_len <= 0) {
        DEBUG_INFO("topic len = 0");
        return;
    }
    uint8* topic = malloc(topic_len*sizeof(uint8)+1);
    if (NULL == topic) {
        DEBUG_INFO("malloc failed topic");
        goto DO_MSG_EXIT;
    }
    memset(topic,0,topic_len*sizeof(uint8)+1);
    memcpy(topic,topic_ptr,topic_len);

    //do msg
    const uint8 *msg_ptr = NULL;
    uint16 msg_len = mqtt_parse_pub_msg_ptr(buf,&msg_ptr);
    if (msg_len <= 0) {
        DEBUG_INFO("msg len = 0");
        goto DO_MSG_EXIT;
    }
    uint8* msg = malloc(msg_len*sizeof(uint8));
    if (NULL == msg) {
        DEBUG_INFO("malloc failed msg");
        goto DO_MSG_EXIT;
    }
    mqtt_parse_publish_msg(buf,msg);
    do_topic_and_msg(topic,msg);
    
DO_MSG_EXIT:  
    if (msg != NULL) {
        free(msg);
    }
    if (topic != NULL) {
        free(topic);
    }
}

/*计算mqtt报文剩余长度*/
int calc_due_length(uint8 *data)
{
	int multiplier =1;
	int len = 0 ;
	uint8_t encodedByte;
	do{
		data++;
		encodedByte = *data;
		len+=(encodedByte & 127)*multiplier;
		multiplier*=128;
		if(multiplier > 128*128*128){
			return -1;
		}
	}while((encodedByte & 128)!=0);
	return len;
}

uint8_t g_read_more_num = 0;
uint8_t g_read_more_data[5];
void do_socket(int sockfd)
{
    unsigned char read_more_num= g_read_more_num;
    unsigned char buf[8193] = {0};
    memcpy(buf,g_read_more_data,read_more_num);
	int ret = read(sockfd,&buf[read_more_num],5-read_more_num);
    if (ret < 1 || ret >(5-read_more_num)){
        perror("read");
        DEBUG_INFO("read");
        if(errno == EINTR/*读取数据过程中被中断*/ || errno == EAGAIN/*缓存区无数据可读,Resource temporarily unavailable*/ 
            || errno == EWOULDBLOCK/*在VxWorks和Windows上，EAGAIN的名字叫做EWOULDBLOCK*/){
		    DEBUG_INFO("EINTR,EAGAIN,EWOULDBLOCK");
			return;
		}
        close(sockfd);
        DEBUG_INFO("client close and exit");
        exit(0);
    }

    int data_length=calc_due_length(buf);
    int length_byte_num=0;
	if(data_length < 128){
		length_byte_num=1;
	}
    else if(data_length < 16384){
		length_byte_num=2;
	}
    else if(data_length < 2097152){
		length_byte_num=3;
	}
    else if(data_length < 268435456){
		length_byte_num=4;
	}
	if(data_length > sizeof(buf)-1){
		g_read_more_num = 0;
		return;
	}

    int ret2 = ret+read_more_num;//当前Data的数据总量
	int read_num_2= data_length-(ret2- (length_byte_num+1));
	if(read_num_2 < 0 ){ //意外读到了下一条mqtt数据(粘包)
		read_more_num = -read_num_2;
		memcpy(g_read_more_data , &buf[ret2-read_more_num] , read_more_num);
		int n;
		g_read_more_num = read_more_num;
	}
    else{
		g_read_more_num = 0;
	}
    int retry_counts=0;
	while(read_num_2>0){
		int byte_read =  read(sockfd ,&buf[ret2] ,read_num_2);
		if(byte_read==0){
			return;
		}
		if(byte_read < 0){
			DEBUG_INFO("Socket Error");
			if(retry_counts++ < 1000){
				DEBUG_INFO("Socket revice error2 byteRead=%d",byte_read); 
				usleep(1000);
				byte_read=0;					
			}
			else{
				DEBUG_INFO("Socket revice overtime"); 
				return;
			}
		}
		read_num_2 -= byte_read;
		ret2 += byte_read;
	}
    
    uint8_t msg_type = MQTTParseMessageType(buf) >> 4;
    DEBUG_INFO("%d",msg_type);
    switch (msg_type) {
      case MSG_CONNACK:
        DEBUG_INFO("MSG_CONNACK:%d",MSG_CONNACK);
      break;

      case MSG_PUBLISH:
        DEBUG_INFO("MSG_PUBLISH:%d",MSG_PUBLISH);
        mqtt_do_publish_msg(buf,sockfd);
      break;

       case MSG_PUBREC:
        mqtt_qos2_pubrel(sockfd,buf,0x62);
        DEBUG_INFO("MSG_PUBREC:%d",MSG_PUBREC);
      break;

      case MSG_PUBREL:
        mqtt_qos2_pubrel(sockfd,buf,0x70);
        DEBUG_INFO("MSG_PUBREL:%d",MSG_PUBREL);
      break;

      case MSG_SUBSCRIBE:
        DEBUG_INFO("MSG_SUBSCRIBE:%d",MSG_SUBSCRIBE);
      break;

      case MSG_SUBACK:
        DEBUG_INFO("MSG_SUBACK:%d",MSG_SUBACK);
      break;
      
      case MSG_UNSUBSCRIBE:
        DEBUG_INFO("MSG_UNSUBSCRIBE:%d",MSG_UNSUBSCRIBE);
      break;

      case MSG_PINGRESP:
        DEBUG_INFO("MSG_PINGRESP:%d",MSG_PINGRESP);
      break;

      case MSG_DISCONNECT:
        DEBUG_INFO("MSG_DISCONNECT:%d",MSG_DISCONNECT);
      break;

      case MSG_UNSUBACK:
        DEBUG_INFO("MSG_UNSUBACK:%d",MSG_UNSUBACK);
      break;
   }
}

int mqtt_connect_init(int sockfd)
{
    int ret =  mqtt_connect(sockfd);
    if(ret < 0){
        DEBUG_INFO("mqtt_connect error");
        return -1;
    }
    DEBUG_INFO("client send connect mqtt sucessful");
    return 0;
}

void *client_do_pthread(void *arg)
{
    pthread_detach(pthread_self());
    g_sockfd = socket_init();
    if (g_sockfd < 0){
        DEBUG_INFO("socket_init error");
        return NULL;
    }
    int ret;//发送三次握手报文
    ret = connect_server(SERVER_PORT,SERVER_IP,g_sockfd);
    if(ret < 0){
        DEBUG_INFO("connect_server error");
        return NULL;
    }
    ret = mqtt_connect_init(g_sockfd); 
    if (ret < 0){
        DEBUG_INFO("mqtt_connect_init failed");
        return NULL;
    }
    
    int i = 0;
    struct pollfd pollfds[1];
    while(1){
    	pollfds[0].fd=g_sockfd;
    	pollfds[0].events=POLLIN;
    	int num = poll(pollfds,1,-1);
        for(i=0; i < num; ++i){
            if(pollfds[i].revents & POLLIN){
    		    do_socket(pollfds[0].fd);
    	    }
        }
    }
}

void *mqtt_ping_pthread(void *arg)
{
    pthread_detach(pthread_self());
    while(1){
        sleep(10);
        mqtt_ping(g_sockfd);
    }
}

void mqtt_qos2_pubrel(int         sockfd , unsigned char *data,unsigned char head_type)
{
    uint16 msg_id = mqtt_parse_msg_id(data);
	unsigned char qos2_pubrel_respon[]={head_type ,0x02 , (msg_id & 0xff00)>>8 , msg_id & 0x00ff};
	client_send(sockfd,qos2_pubrel_respon,sizeof(qos2_pubrel_respon));
}

