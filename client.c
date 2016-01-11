#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <errno.h>
#include <netdb.h>


typedef struct _sock_data {
	int sockfd;//句柄
	int ret ;//错误类型
	char recv_buff[1024];//收到数据的缓存
	int recv_byte;//收到数据的长度
	struct sockaddr_in server_addr;//socket地址
	char ip[24];//ip地址
	int port;//端口号
	int mode;//阻塞类型
	fd_set read_set,write_set, err_set;//读取结果集
	struct timeval timeval ;//超时
}t_sock_data;

//收到数据的回调
typedef void (*recv_data_func)(t_sock_data *sc_data);

/**
 *向服务器发送数据
 */
void *send2server(t_sock_data *sc_data) {
	if(NULL == sc_data) {
		printf("send2server : sc_data is null");
		return NULL;
	}

	char input[1024] = {0};

	while(1) {
		bzero(input,sizeof(input));
		scanf("%s",input);
		//input[strlen(input)] = '\r';
		//input[strlen(input)] = '\n';
		//sprintf(input,"send : %s \r\n",input);
		printf("send : %s , %d \n",input,sc_data->sockfd);
		write(sc_data->sockfd,input,strlen(input));
		usleep(100);
		if(0 == strcmp(input,"q")) {
			close(sc_data->sockfd);
		}
	}
}

/**
 * 连接到服务器
 */
void conn2server(t_sock_data *sc_data,recv_data_func recv_func)
{
	struct hostent *host = NULL;
	int mode = 0;
	char ip_str[32] = {0};

	if(NULL == sc_data) {
		sc_data->ret = EXIT_FAILURE;
		return;
	}
	//初始化
	sc_data->sockfd = 0;
	sc_data->ret = EXIT_SUCCESS;
	bzero(sc_data->recv_buff,sizeof(sc_data->recv_buff));

	//gethostbyname可以通过主机名称得到主机的IP地址
	host = gethostbyname((const char *)sc_data->ip);
	if(NULL == host) {
		printf("Gethostname error\n");
		sc_data->ret = EXIT_FAILURE;
		return;
	}
	
	// 客户程序开始建立 sockfd描述符
	sc_data->sockfd = socket(AF_INET,SOCK_STREAM,0);
	if(-1 == sc_data->sockfd) {
		printf("Socket Error:%s\a\n",strerror(errno));
		goto error;
	}

	// 客户程序填充服务端的资料
	bzero(&(sc_data->server_addr),sizeof(sc_data->server_addr));
	sc_data->server_addr.sin_family = AF_INET;
	
	// 主机字节序转换为网络字节序
	sc_data->server_addr.sin_port = htons(sc_data->port);
	inet_ntop(host->h_addrtype, *(host->h_addr_list), ip_str, sizeof(ip_str));
	sc_data->server_addr.sin_addr.s_addr = inet_addr(ip_str);
	
	mode = 1;
	sc_data->ret = ioctl(sc_data->sockfd, FIONBIO, &mode);
    if (EXIT_SUCCESS != sc_data->ret) {
        printf("ioctlsocket failed with error: %d,reason : %s\n", sc_data->ret,strerror(errno));
		goto error;
	}

	printf("Start conn %s : %d\n",ip_str,sc_data->port);
	
	// 客户程序发起连接请求 
	sc_data->ret = connect(sc_data->sockfd,(struct sockaddr *)(&sc_data->server_addr),sizeof(struct sockaddr));
	if(-1 == sc_data->ret) {
		// 超时
		FD_ZERO(&sc_data->write_set);
		FD_ZERO(&sc_data->err_set);
		FD_ZERO(&sc_data->read_set);
		FD_SET(sc_data->sockfd, &sc_data->write_set);
		FD_SET(sc_data->sockfd, &sc_data->err_set);
		FD_SET(sc_data->sockfd, &sc_data->read_set);
	
		// check if the socket is ready
		sc_data->ret = select(sc_data->sockfd + 1, &sc_data->read_set, &sc_data->write_set, &sc_data->err_set, &sc_data->timeval);
		if(-1 == sc_data->ret) {
			printf("Conn failed %s !!!\n",strerror(errno));
			goto error;
		} else if (0 == sc_data->ret) {
			printf("Conn failed timeout !!!\n");
			goto error;
		} else {
			if(FD_ISSET(sc_data->sockfd, &(sc_data->write_set))) {
				mode = 0;
				sc_data->ret = ioctl(sc_data->sockfd, FIONBIO, &mode);
			    if (EXIT_SUCCESS != sc_data->ret ) {
					printf("ioctlsocket failed with error: %d , %s\n", sc_data->ret , strerror(errno) );
					goto error;
				}
				// 连接成功了 
				printf("Conn success !!!\n");
			} else {
				printf("Conn failed !!!\n");
				goto error;
			}
		}		
	}

	
	//开始接收数据
	while(1) {
		
		FD_ZERO(&sc_data->read_set);
		FD_SET(sc_data->sockfd, &sc_data->read_set);
		
		sc_data->ret = select(sc_data->sockfd + 1, &sc_data->read_set, NULL, NULL, &sc_data->timeval);
		if(-1 == sc_data->ret) {
			printf("select failed after connected %s !!! \n",strerror(errno));
			goto error;
		} else if (0 == sc_data->ret) {
			continue;
		} else {
			if ((sc_data->ret = FD_ISSET(sc_data->sockfd, &sc_data->read_set)) > 0) { //socket中获取数据
				bzero(sc_data->recv_buff,sizeof(sc_data->recv_buff));
				sc_data->recv_byte = read(sc_data->sockfd , sc_data->recv_buff , sizeof(sc_data->recv_buff) - 1);
				if(0 >= sc_data->recv_byte) {
					printf("Read Error:%s\n",strerror(errno));
					goto error;
				} else {
					printf("recv len = %d\n",sc_data->recv_byte);
				}
				if(NULL != recv_func) {
					recv_func(sc_data);
				}
			} else {
				printf("FD_ISSET failed after connected %d , %s!!!\n",sc_data->ret , strerror(errno));
				goto error;
			}
		}		
	}

//错误
error:
	close(sc_data->sockfd);
	if(EXIT_SUCCESS == sc_data->ret) {
		sc_data->ret = EXIT_FAILURE;
	}
	return;
}

void recv_from_server(t_sock_data *sc_data) {
	if(NULL == sc_data) {
		return;
	}


	sc_data->recv_buff[sc_data->recv_byte]='\0';
	printf("cb :I have received :%d, %s\n",sc_data->sockfd, sc_data->recv_buff);
}

int main(int argc, char *argv[])
{
	int ret = EXIT_SUCCESS;
	int sockfd;
	char buffer[1024] = {0};
	struct sockaddr_in server_addr;
	struct hostent *host = NULL;
	int portnumber,nbytes;
	char *ip = NULL;
	int mode = 1;
	fd_set read_set,write_set, err_set;

	struct timeval timeval = {0};
    timeval.tv_sec = 3;
    timeval.tv_usec = 0;

	t_sock_data sc_data;

	//ip 端口
	bzero(sc_data.ip,sizeof(sc_data.ip));
	strcpy(sc_data.ip,"192.168.1.111");
	sc_data.port = 30001;

	//设置超时
	sc_data.timeval.tv_sec = 3;
	sc_data.timeval.tv_usec = 0;
	
	//连接
	conn2server(&sc_data,recv_from_server);

	pthread_t thread;
	pthread_create(&thread,NULL,(void *)send2server,(void *)sc_data);

error:
	close(sockfd);
	return EXIT_FAILURE;
}

