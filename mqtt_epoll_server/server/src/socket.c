#include "socket.h"

socket_record_t *socket_record_head = NULL;

void delete_sockfd_record(int sockfd)
{
	socket_record_t *src_record, *pre_record = NULL;
	src_record = socket_record_head;
	while ((src_record->sockfd != sockfd) && (src_record->next)) {
		pre_record = src_record;
		src_record = src_record->next;
	}

	if (src_record->sockfd != sockfd) {
		return;
	}
    //DEBUG_INFO("client %s close",inet_ntoa((src_record->client_addr).sin_addr));
    DEBUG_INFO("client close");
	if (src_record) {
		if (pre_record == NULL) {
			return;
		}
		pre_record->next = src_record->next;
        mqtt_epoll_del(epfd, src_record->sockfd); 
		close(src_record->sockfd);
		free(src_record);
	}
}

int accept_connect(void)
{
	int tr = 1;
	socket_record_t *src_record;
	socket_record_t *new_sockfd = calloc(1 , sizeof(socket_record_t));
    new_sockfd->client_addr_len = sizeof(new_sockfd->client_addr);
	new_sockfd->sockfd = accept(socket_record_head->sockfd,(struct sockaddr *) &(new_sockfd->client_addr), &(new_sockfd->client_addr_len));
	if (new_sockfd->sockfd < 0) {
		DEBUG_INFO("ERROR on accept");
		free(new_sockfd);
		return -1;
	}
    //DEBUG_INFO("client %s is connect",inet_ntoa((new_sockfd->client_addr).sin_addr));
    DEBUG_INFO("client connect");
    mqtt_epoll_add(epfd,new_sockfd->sockfd);
    set_linger(new_sockfd->sockfd,1);
	// "Address Already in Use" error on the bind
	setsockopt(new_sockfd->sockfd, SOL_SOCKET, SO_REUSEADDR, &tr, sizeof(int));
    /*非阻塞*/
	fcntl(new_sockfd->sockfd, F_SETFL, O_NONBLOCK);
	new_sockfd->next = NULL;
    new_sockfd->publish_seq = 0;
	src_record = socket_record_head;
    /*链表采用尾插法*/
	while (src_record->next){
		src_record = src_record->next;
	}
	src_record->next = new_sockfd;
	return (new_sockfd->sockfd);
}

int socket_init(int port)
{
    struct sockaddr_in server_addr;
    int stat, tr = 1;

    if (NULL == socket_record_head) { 
        socket_record_t *sockfd_record = malloc(sizeof(socket_record_t));
        sockfd_record->sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd_record->sockfd < 0) {
            DEBUG_INFO("ERROR opening socket");
            return -1;
        }
        DEBUG_INFO("lsSocket->sockfd:%d",sockfd_record->sockfd);
        setsockopt(sockfd_record->sockfd, SOL_SOCKET, SO_REUSEADDR, &tr, sizeof(int));
        // Set the fd to none blocking
        fcntl(sockfd_record->sockfd, F_SETFL, O_NONBLOCK);
        bzero((char *) &server_addr, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(port);
        stat = bind(sockfd_record->sockfd, (struct sockaddr*) &server_addr,sizeof(server_addr));
        if (stat < 0) {
            DEBUG_INFO("ERROR on binding: %s\n", strerror(errno));
            close(sockfd_record->sockfd);
            return -1;
        }
        listen(sockfd_record->sockfd, 5);
        sockfd_record->next = NULL;
        socket_record_head = sockfd_record;
        epfd = mqtt_epoll_create(0);
        if (epfd < 0)  {
           perror("epoll_create");
           exit(-1);
        }
        mqtt_epoll_add(epfd,socket_record_head->sockfd);
    }
    return 0;
} 

int send_msg(int fd, void *buf, size_t n) 
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
	    send_msg(sockfd,qos1_respon,sizeof(qos1_respon));
    }
    else if(qos == 2){
        //messge id
        uint16 msg_id = mqtt_parse_msg_id(buf);
        uint8 qos2_respon[]={0x50 ,0x02 , (msg_id & 0xff00) >> 8, msg_id & 0x00ff};
	    send_msg(sockfd,qos2_respon,sizeof(qos2_respon));
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

    char server_msg[] = {"server publish msg"};
    mqtt_publish(sockfd,"server reply theme",server_msg,strlen(server_msg),0);
    
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
	uint8_t encode_byte;
	do{
		data++;
		encode_byte = *data;
		len+=(encode_byte & 127)*multiplier;
		multiplier*=128;
		if(multiplier > 128*128*128){
			return -1;
		}
	}while((encode_byte & 128)!=0);
	return len;
}

socket_record_t *look_up_by_sokfd(int sockfd)
{
	socket_record_t * src_record;
	src_record = socket_record_head;
	while (src_record!=NULL) {
		if(src_record->sockfd == sockfd){
			return src_record;
		}
		else{
			src_record=src_record->next;
		}
	}
	return NULL;
}

void do_socket(int sockfd)
{
    socket_record_t *sockfd_recored = look_up_by_sokfd(sockfd);
    if(NULL == sockfd_recored){
        return;
    }
    unsigned char read_more_num= sockfd_recored->read_more_num;
    unsigned char buf[8193] = {0};
    memcpy(buf,sockfd_recored->read_more_data,read_more_num);
    //有些报文的固定头+剩余长度+负载会小于5字节，所以有可能读5字节产生沾包
	int ret = read(sockfd,&buf[read_more_num],5-read_more_num);
    if (ret < 1 || ret >(5-read_more_num)){
        perror("read");
        DEBUG_INFO("read");
        if(errno == EINTR/*读取数据过程中被中断*/ || errno == EAGAIN/*缓存区无数据可读,Resource temporarily unavailable*/ 
            || errno == EWOULDBLOCK/*在VxWorks和Windows上，EAGAIN的名字叫做EWOULDBLOCK*/){
		    DEBUG_INFO("EINTR,EAGAIN,EWOULDBLOCK");
			return;
		}
        delete_sockfd_record(sockfd);
        return;
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
		sockfd_recored->read_more_num = 0;
		return;
	}

    /*
    比如pingreq与publish的报文可以会一起读，需要把多读取的publish报文先保存起来，下次再读取
    如果不沾包的话，
    Data数据总量（ret2）- 报文头字节数（1）- 报文头外报文剩余长度占用字节数（length_byte_num）
    是会等于 报文头外报文剩余长度的
    */
    int ret2 = ret+read_more_num;//当前Data的数据总量
	int read_num_2= data_length/*可变报报头+负载*/-(ret2- (length_byte_num/*剩余长度占用字节数*/+1/*一个字节报文头*/));
	if(read_num_2 < 0 ){ //意外读到了下一条mqtt数据(粘包)
		read_more_num = -read_num_2;
        //把多读的报文先保存起来
		memcpy(sockfd_recored->read_more_data , &buf[ret2-read_more_num] , read_more_num);
		int n;
		sockfd_recored->read_more_num = read_more_num;
	}
    else{
		sockfd_recored->read_more_num = 0;
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
      case MSG_CONNECT:
        DEBUG_INFO("MSG_CONNECT:%d",MSG_CONNECT);
        mqtt_connect_ack(sockfd);
      break;
      
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
        mqtt_subscribe_ack(sockfd,buf);
      break;

      case MSG_SUBACK:
        DEBUG_INFO("MSG_SUBACK:%d",MSG_SUBACK);
      break;
      
      case MSG_UNSUBSCRIBE:
        DEBUG_INFO("MSG_UNSUBSCRIBE:%d",MSG_UNSUBSCRIBE);
        mqtt_unsubscribe_ack(sockfd,buf);
      break;

      case MSG_PINGREQ:
        DEBUG_INFO("MSG_PINGRESP:%d",MSG_PINGREQ);
        mqtt_ping_req_reply(sockfd);
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

void *server_do_pthread(void *arg)
{
   pthread_detach(pthread_self());
   int nfds = -1;
   int i;
   struct epoll_event events[MAXEVENTS];
	while(1) {
		/*-1永久阻塞*/
        nfds = epoll_wait(epfd, events, MAXEVENTS/*一次处理的最大事件数,也就是fd数*/, -1);
		for(i = 0; i < nfds; ++i) {
            //DEBUG_INFO("%d,%d",events[i].data.fd,socket_record_head->sockfd);
        	if (events[i].data.fd == socket_record_head->sockfd) {
                accept_connect();
			}
			else if(events[i].events & EPOLLIN) {
            	if (events[i].data.fd < 0) {
                    continue;
            	}
				do_socket(events[i].data.fd);
			}
			else if(events[i].events & EPOLLHUP) {
                DEBUG_INFO("EPOLLHUP");
                delete_sockfd_record(events[i].data.fd);
			}
			else if(events[i].events & EPOLLERR) {
				DEBUG_INFO("EPOLLERR");
                delete_sockfd_record(events[i].data.fd);
			}
		}
	}
}

void mqtt_qos2_pubrel(int         sockfd , unsigned char *data,unsigned char head_type)
{
    uint16 msg_id = mqtt_parse_msg_id(data);
	unsigned char qos2_pubrel_respon[]={head_type ,0x02 , (msg_id & 0xff00)>>8 , msg_id & 0x00ff};
	send_msg(sockfd,qos2_pubrel_respon,sizeof(qos2_pubrel_respon));
}

