#include "t_client.h"

/**
 * 释放资源
 */
void t_free(void *data){
	if (NULL == data) {
		free(data);
		data = NULL;
	}
}

/**
 * HEX转字符串
 */
char * hex_2_str(char *hex,int len) {
	char *res = NULL;
	char *tmp = hex;
	int i = 0;

	if ((NULL == hex) || (len <= 0)) {
		return res;
	}

	res = (char *)malloc(len * 2 + 1);
	bzero(res,len * 2 + 1);
	for (i = 0 ;i < len;i++) {
		sprintf(res,"%s%2X",res,*tmp);
		tmp++;
	}

	return res;
}


/**
 * 连接到服务器
 */
void conn2server(t_sock_data *sc_data,recv_data_func recv_func)
{
	struct sockaddr_in server_addr;//socket地址
	int mode;//阻塞类型
	fd_set read_set_bak,read_set,write_set, err_set;//读取结果集
	struct hostent *host = NULL;
	char ip_str[32] = {0};
	char recv_buff[MAX_BUFF_LEN] = {0};
	char recv_len = 0;

	if(NULL == sc_data) {
		sc_data->ret = EXIT_FAILURE;
		return;
	}

	//初始化
	sc_data->sockfd = 0;
	sc_data->ret = EXIT_SUCCESS;
	bzero(recv_buff,sizeof(recv_buff));

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
	bzero(&(server_addr),sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	
	// 主机字节序转换为网络字节序
	server_addr.sin_port = htons(sc_data->port);
	inet_ntop(host->h_addrtype, *(host->h_addr_list), ip_str, sizeof(ip_str));
	server_addr.sin_addr.s_addr = inet_addr(ip_str);
	
	//转为非阻塞模式
	mode = 1;
	sc_data->ret = ioctl(sc_data->sockfd, FIONBIO, &mode);
    if (EXIT_SUCCESS != sc_data->ret) {
        printf("ioctlsocket failed with error: %d,reason : %s\n", sc_data->ret,strerror(errno));
		goto error;
	}

	printf("Start conn %s : %d\n",ip_str,sc_data->port);
	
	// 客户程序发起连接请求 
	sc_data->ret = connect(sc_data->sockfd,(struct sockaddr *)(&server_addr),sizeof(struct sockaddr));
	if(-1 == sc_data->ret) {
		// 超时
		FD_ZERO(&write_set);
		FD_ZERO(&err_set);
		FD_ZERO(&read_set);
		FD_SET(sc_data->sockfd, &write_set);
		FD_SET(sc_data->sockfd, &err_set);
		FD_SET(sc_data->sockfd, &read_set);
	
		// 检测socket是否成功
		sc_data->ret = select(sc_data->sockfd + 1, &read_set, &write_set, &err_set, &sc_data->timeval);
		if(-1 == sc_data->ret) {
			printf("Conn failed %s !!!\n",strerror(errno));
			goto error;
		} else if (0 == sc_data->ret) {
			printf("Conn failed timeout !!!\n");
			goto error;
		} else if(FD_ISSET(sc_data->sockfd, &(write_set))) {
				//转为阻塞模式
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
	} else {
		// 连接成功了 
		printf("Conn success soon !!!\n");
		
	}

	
	FD_ZERO(&read_set);
	FD_SET(sc_data->sockfd, &read_set);
	//开始接收数据
	while(1) {
		read_set_bak = read_set;	
		
		sc_data->ret = select(sc_data->sockfd + 1, &read_set_bak, NULL, NULL, &sc_data->timeval);
		if(-1 == sc_data->ret) {
			printf("select failed after connected %s !!! \n",strerror(errno));
			goto error;
		} else if (0 == sc_data->ret) {
			continue;
		} else {
			if ((sc_data->ret = FD_ISSET(sc_data->sockfd, &read_set_bak)) > 0) { //socket中获取数据
				bzero(recv_buff,sizeof(recv_buff));
				recv_len = read(sc_data->sockfd , recv_buff , sizeof(recv_buff) - 1);
				if(0 >= recv_len) {
					printf("Read Error:%s\n",strerror(errno));
					goto error;
				} else {
					//调用回调
					if(NULL != recv_func) {
						recv_func(sc_data,recv_buff,recv_len);
					}
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

