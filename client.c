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


/**
 * 连接到服务器
 */
int conn2server(const char* ip,int port,int sockfd,int timeout)
{
	int ret = EXIT_SUCCESS;

	

	return ret;
}


typedef struct _sock_data {
	int sockfd;
	int ret;
}sock_data;


void *input2server(sock_data *sc_data) {
	if(NULL == sc_data) {
		return NULL;
	}

	char input[1024] = {0};
	
	while(1) {
		bzero(input,sizeof(input));
		scanf("%s",input);
		//input[strlen(input)] = '\r';
		//input[strlen(input)] = '\n';
		write(sc_data->sockfd,input,strlen(input));
		if(0 == strcmp(input,"q")) {
			close(sc_data->sockfd);
		}
	}
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
	int result = EXIT_SUCCESS;
	fd_set read_set,write_set, err_set;

	struct timeval timeval = {0};
    timeval.tv_sec = 3;
    timeval.tv_usec = 0;

	if(argc != 3) {
		printf("Usage:%s hostname portnumber\a\n",argv[0]);
		return EXIT_FAILURE;
	}
	
	//gethostbyname可以通过主机名称得到主机的IP地址
	ip = argv[1];
	host = gethostbyname((const char *)ip);
	if(host == NULL) {
		printf("Gethostname error\n");
		return EXIT_FAILURE;
	}
	
	//portnumber为端口号
	if(0 == (portnumber=atoi(argv[2]))) {
		printf("Usage:%s hostname portnumber\a\n",argv[0]);
		return EXIT_FAILURE;
	}
	
	// 客户程序开始建立 sockfd描述符
	sockfd = socket(AF_INET,SOCK_STREAM,0);
	if(-1 == sockfd)
	{
		printf("Socket Error:%s\a\n",strerror(errno));
		goto error;
	}

	// 客户程序填充服务端的资料
	bzero(&server_addr,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	
	// 主机字节序转换为网络字节序
	server_addr.sin_port = htons(portnumber);
	server_addr.sin_addr.s_addr = inet_addr(argv[1]);
	
	mode = 1;
	result = ioctl(sockfd, FIONBIO, &mode);
    if (EXIT_SUCCESS != result) {
        printf("ioctlsocket failed with error: %d,reason : %s\n", result,strerror(errno));
		goto error;
	}

	printf("Start conn\n");
	
	// 客户程序发起连接请求 
	result = connect(sockfd,(struct sockaddr *)(&server_addr),sizeof(struct sockaddr));
	if(-1 == result) {
		// 超时
		FD_ZERO(&write_set);
		FD_ZERO(&err_set);
		FD_ZERO(&read_set);
		FD_SET(sockfd, &write_set);
		FD_SET(sockfd, &err_set);
		FD_SET(sockfd, &read_set);
	
		// check if the socket is ready
		result = select(sockfd + 1, &read_set, &write_set, &err_set, &timeval);
		if(-1 == result) {
			printf("Conn failed %s !!!\n",strerror(errno));
			goto error;
		} else if (0 == result) {
			printf("Conn failed timeout !!!\n");
			goto error;
		} else {
			if(FD_ISSET(sockfd, &write_set)) {
				mode = 0;
				result = ioctl(sockfd, FIONBIO, &mode);
			    if (EXIT_SUCCESS != result ) {
					printf("ioctlsocket failed with error: %d , %s\n", result , strerror(errno) );
					return EXIT_FAILURE;
				}
				// 连接成功了 
				printf("Conn success !!!\n");
			} else {
				printf("Conn failed !!!\n");
				goto error;
			}
		}		
	}

	const char * write2server = "panda";

	//开始收发数据
	while(1) {
		
		FD_ZERO(&read_set);
		FD_SET(sockfd, &read_set);
		FD_SET(0, &read_set);
		
		
		result = select(sockfd + 1, &read_set, NULL, NULL, &timeval);
		if(-1 == result) {
			printf("select failed after connected %s !!! \n",strerror(errno));
			goto error;
		} else if (0 == result) {
			continue;
		} else {
			if ((result = FD_ISSET(sockfd, &read_set)) > 0) { //socket中获取数据
				bzero(buffer,sizeof(buffer));
				nbytes = read(sockfd , buffer , sizeof(buffer) - 1);
				if(0 >= nbytes) {
					printf("Read Error:%s\n",strerror(errno));
					goto error;
				} else {
					printf("recv len = %d\n",nbytes);
				}
				buffer[nbytes]='\0';
				printf("I have received:%s\n",buffer);
			} else if ((result = FD_ISSET(0, &read_set)) > 0) {//从键盘输入
				bzero(buffer, sizeof(buffer));
				fgets(buffer, sizeof(buffer), stdin);  
				write(sockfd, buffer, strlen(buffer));
			} else {
				printf("FD_ISSET failed after connected %d , %s!!!\n",result , strerror(errno));
				goto error;
			}
		}		
	}

	// 结束通讯 
	close(sockfd);
	return EXIT_SUCCESS;

error:
	close(sockfd);
	return EXIT_FAILURE;
}

