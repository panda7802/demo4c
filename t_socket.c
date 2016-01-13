#include "t_socket.h"

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
void conn2server(t_client_conn *cli_conn,t_cli_func sock_func)
{
	struct sockaddr_in server_addr;//socket地址
	int mode;//阻塞类型
	fd_set read_set_bak,read_set,write_set, err_set;//读取结果集
	struct hostent *host = NULL;
	char ip_str[32] = {0};
	char recv_buff[MAX_BUFF_LEN] = {0};
	char recv_len = 0;

	if(NULL == cli_conn) {
		printf("cli_conn is null\n");
		cli_conn->ret = TS_ERR_PARM;
		return;
	}

	//初始化
	cli_conn->sockfd = 0;
	cli_conn->ret = EXIT_SUCCESS;
	bzero(recv_buff,sizeof(recv_buff));

	//gethostbyname可以通过主机名称得到主机的IP地址
	host = gethostbyname((const char *)cli_conn->ip);
	if (NULL == host) {
		printf("Gethostname error\n");
		cli_conn->ret = TS_ERR_PARM;
		return;
	}
	
	// 客户程序开始建立 sockfd描述符
	cli_conn->sockfd = socket(AF_INET,SOCK_STREAM,0);
	if (-1 == cli_conn->sockfd) {
		printf("Socket Error:%s\a\n",strerror(errno));
		goto error;
	}

	// 客户程序填充服务端的资料
	bzero(&(server_addr),sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	
	// 主机字节序转换为网络字节序
	server_addr.sin_port = htons(cli_conn->port);
	inet_ntop(host->h_addrtype, *(host->h_addr_list), ip_str, sizeof(ip_str));
	server_addr.sin_addr.s_addr = inet_addr(ip_str);
	
	//转为非阻塞模式
	mode = 1;
	cli_conn->ret = ioctl(cli_conn->sockfd, FIONBIO, &mode);
    if (EXIT_SUCCESS != cli_conn->ret) {
        printf("ioctlsocket failed with error: %d,reason : %s\n", cli_conn->ret,strerror(errno));
		goto error;
	}

	printf("Start conn %s : %d\n",ip_str,cli_conn->port);
	
	// 客户程序发起连接请求 
	cli_conn->ret = connect(cli_conn->sockfd,(struct sockaddr *)(&server_addr),sizeof(struct sockaddr));
	if(-1 == cli_conn->ret) {
		// 超时
		FD_ZERO(&write_set);
		FD_ZERO(&err_set);
		FD_ZERO(&read_set);
		FD_SET(cli_conn->sockfd, &write_set);
		FD_SET(cli_conn->sockfd, &err_set);
		FD_SET(cli_conn->sockfd, &read_set);
	
		// 检测socket是否成功
		cli_conn->ret = select(cli_conn->sockfd + 1, &read_set, &write_set, &err_set, &cli_conn->timeval);
		if(-1 == cli_conn->ret) {
			printf("Conn failed %s !!!\n",strerror(errno));
			goto error;
		} else if (0 == cli_conn->ret) {
			cli_conn->ret = TS_ERR_TIMIEOUT;
			printf("Conn failed timeout !!!\n");
			goto error;
		} else if(FD_ISSET(cli_conn->sockfd, &(write_set))) {
				//转为阻塞模式
				mode = 0;
				cli_conn->ret = ioctl(cli_conn->sockfd, FIONBIO, &mode);
			    if (EXIT_SUCCESS != cli_conn->ret ) {
					printf("ioctlsocket failed with error: %d , %s\n", cli_conn->ret , strerror(errno) );
					goto error;
				}
				// 连接成功了 
				printf("Conn success !!!\n");
				cli_conn->ret = TS_CONN;
				if(NULL != sock_func.on_conn_status) {
					sock_func.on_conn_status(cli_conn);
				}
		} else {
				printf("Conn failed !!!\n");
				goto error;
		}		
	} else {
		// 连接成功了 
		printf("Conn success soon !!!\n");
	}
	
	FD_ZERO(&read_set);
	FD_SET(cli_conn->sockfd, &read_set);
	//开始接收数据
	while(1) {
		read_set_bak = read_set;	
		
		cli_conn->ret = select(cli_conn->sockfd + 1, &read_set_bak, NULL, NULL, &cli_conn->timeval);
		if(-1 == cli_conn->ret) {
			printf("select failed after connected %s !!! \n",strerror(errno));
			goto error;
		} else if (0 == cli_conn->ret) {
			continue;
		} else {
			if ((cli_conn->ret = FD_ISSET(cli_conn->sockfd, &read_set_bak)) > 0) { //socket中获取数据
				bzero(recv_buff,sizeof(recv_buff));
				recv_len = read(cli_conn->sockfd , recv_buff , sizeof(recv_buff) - 1);
				if(0 >= recv_len) {
					printf("Read Error:%s\n",strerror(errno));
					goto error;
				} else {
					//调用回调
					if(NULL != sock_func.on_recv_data) {
						sock_func.on_recv_data(cli_conn,recv_buff,recv_len);
					}
				}
			} else {
				printf("FD_ISSET failed after connected %d , %s!!!\n",cli_conn->ret , strerror(errno));
				goto error;
			}
		}		
	}

//错误
error:
	if(EXIT_SUCCESS == cli_conn->ret) {
		cli_conn->ret = TS_ERR_NET;
		cli_conn->sys_err = errno;
	}
	if(NULL != sock_func.on_conn_status) {
		sock_func.on_conn_status(cli_conn);
	}
	close(cli_conn->sockfd);
	cli_conn->ret = TS_ERR_NET;
	cli_conn->sys_err = errno;
	if(NULL != sock_func.on_conn_status) {
		sock_func.on_conn_status(cli_conn);
	}

	return;
}

/**
 * 开启服务器
 */
void start_server(t_server_conn *srv_conn,t_srv_func srv_func)
{
	struct servent *servp;
	struct sockaddr_in server, remote;
	int request_sock, new_sock;
	int nfound, fd, maxfd, bytesread;
	socklen_t addrlen;
	fd_set rmask, mask;
	struct timeval timeout = srv_conn->timeval;
	char buf[MAX_BUFF_LEN] = {0};
	int port = srv_conn->port;

	// 创建连接
	if ((request_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		srv_conn->ret = TS_ERR_PARM;
		srv_conn->sys_err = errno;
		srv_func.conn_status_func(srv_conn);
		return;
	}

	// 绑定端口
	memset((void *) &server, 0, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(port);
	if (bind(request_sock, (struct sockaddr *)&server, sizeof server) < 0) {
		perror("bind");
		srv_conn->ret = TS_ERR_PARM;
		srv_conn->sys_err = errno;
		srv_func.conn_status_func(srv_conn);
		return;
	}

	// 监听端口
	if (listen(request_sock, SOMAXCONN) < 0) {
		perror("listen");
		srv_conn->ret = TS_ERR_PARM;
		srv_conn->sys_err = errno;
		srv_func.conn_status_func(srv_conn);
		return;
	}

	FD_ZERO(&mask);
	FD_SET(request_sock, &mask);
	maxfd = request_sock;
	
	printf("start listen server  %d\n",port);
	
	// 循环连接
	for (;;) {
		rmask = mask;
		nfound = select(maxfd + 1, &rmask, NULL, NULL, &timeout);
		if (nfound < 0) {
			if (EINTR == errno) {
				printf("interrupted system call\n");
				continue;
			}
			// 连接出现异常
			perror("select");
			srv_conn->ret = TS_ERR_NET;
			srv_conn->sys_err = errno;
			srv_func.conn_status_func(srv_conn);
			return;
		} else if (nfound == 0) {
			continue;
		}

		if (FD_ISSET(request_sock, &rmask)) {
			// 新的连接
			addrlen = sizeof(remote);
			new_sock = accept(request_sock, (struct sockaddr *)&remote,&addrlen);
			if (new_sock < 0) {
				perror("accept");
				srv_conn->ret = TS_ERR_NET;
				srv_conn->sys_err = errno;
				srv_func.conn_status_func(srv_conn);
				return;
			}
			printf("connection from host %s, port %d, socket %d\n",(char *)inet_ntoa(remote.sin_addr), ntohs(remote.sin_port),new_sock);

			FD_SET(new_sock, &mask);
			if (new_sock > maxfd) {
				maxfd = new_sock;
			}
			FD_CLR(request_sock, &rmask);
		}

		for (fd=0; fd <= maxfd ; fd++) {
			// 循环查询连接的数据
			if (FD_ISSET(fd, &rmask)) {
				bytesread = read(fd, buf, sizeof(buf) - 1);
				if (bytesread < 0) {
					//
					perror("read");
					srv_conn->ret = TS_ERR_NET;
					srv_conn->sys_err = errno;
					srv_func.conn_status_func(srv_conn);
					return;
				} else if (bytesread<=0) {
					// 连接断开
					printf("conn brocken, fd:%d\n",fd);
					FD_CLR(fd, &mask);
					if (close(fd)) {
						perror("close");
						srv_conn->ret = TS_ERR_NET;
						srv_conn->sys_err = errno;
						srv_func.conn_status_func(srv_conn);
						return;
					}
					continue;
				} else {
					srv_func.recv_data_func(srv_conn,fd,buf,bytesread);
				}
			} 
		} 
	} 
}

