#include "t_socket.h"
#include <arpa/inet.h>

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
 * 注意，此处内存未释放
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
void start_tcp_client(t_client_conn *cli_conn,t_cli_func sock_func)
{
	struct sockaddr_in server_addr;//socket地址
	int mode;//阻塞类型
	fd_set read_set_bak,read_set,write_set, err_set;//读取结果集
	struct hostent *host = NULL;
	char ip_str[32] = {0};
	char recv_buf[MAX_buf_LEN] = {0};
	int recv_len = 0;

	if (NULL == cli_conn) {
		printf("cli_conn is null\n");
		cli_conn->ret = TS_ERR_PARM;
		return;
	}

	//初始化
	cli_conn->sockfd = 0;
	cli_conn->ret = EXIT_SUCCESS;
	bzero(recv_buf,sizeof(recv_buf));

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
	if (-1 == cli_conn->ret) {
		// 超时
		FD_ZERO(&write_set);
		FD_ZERO(&err_set);
		FD_ZERO(&read_set);
		FD_SET(cli_conn->sockfd, &write_set);
		FD_SET(cli_conn->sockfd, &err_set);
		FD_SET(cli_conn->sockfd, &read_set);
	
		// 检测socket是否成功
		cli_conn->ret = select(cli_conn->sockfd + 1, &read_set, &write_set, &err_set, &cli_conn->timeval);
		if (-1 == cli_conn->ret) {
			printf("Conn failed %s !!!\n",strerror(errno));
			goto error;
		} else if (0 == cli_conn->ret) {
			cli_conn->ret = TS_ERR_TIMIEOUT;
			printf("Conn failed timeout !!!\n");
			goto error;
		} else if (FD_ISSET(cli_conn->sockfd, &(write_set))) {
			//转为阻塞模式
			mode = 0;
			cli_conn->ret = ioctl(cli_conn->sockfd, FIONBIO, &mode);
			if (EXIT_SUCCESS != cli_conn->ret ) {
				printf("ioctlsocket failed with error: %d , %s\n", cli_conn->ret , strerror(errno) );
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
	
	// 调用
	cli_conn->ret = TS_CONN;
	if (NULL != sock_func.on_conn_status) {
		sock_func.on_conn_status(cli_conn);
	}
	
	FD_ZERO(&read_set);
	FD_SET(cli_conn->sockfd, &read_set);
	//开始接收数据
	while(1) {
		read_set_bak = read_set;	
		
		cli_conn->ret = select(cli_conn->sockfd + 1, &read_set_bak, NULL, NULL, &cli_conn->timeval);
		if (-1 == cli_conn->ret) {
			printf("select failed after connected %s !!! \n",strerror(errno));
			goto error;
		} else if (0 == cli_conn->ret) {
			continue;
		} else {
			if ((cli_conn->ret = FD_ISSET(cli_conn->sockfd, &read_set_bak)) > 0) { //socket中获取数据
				bzero(recv_buf,sizeof(recv_buf));
				recv_len = read(cli_conn->sockfd , recv_buf , sizeof(recv_buf));
				if (0 >= recv_len) {
					printf("Read Error:%s\n",strerror(errno));
					goto error;
				} else {
					//调用回调
					if (NULL != sock_func.on_recv_data) {
						sock_func.on_recv_data(cli_conn,recv_buf,recv_len);
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
	if (EXIT_SUCCESS == cli_conn->ret) {
		cli_conn->ret = TS_ERR_NET;
		cli_conn->sys_err = errno;
	}
	if (NULL != sock_func.on_conn_status) {
		sock_func.on_conn_status(cli_conn);
	}
	if (cli_conn->sockfd > 0) {
		close(cli_conn->sockfd);
	}
	cli_conn->ret = TS_ERR_NET;
	cli_conn->sys_err = errno;
	if (NULL != sock_func.on_conn_status) {
		sock_func.on_conn_status(cli_conn);
	}

	return;
}

/**
 * 开启服务器
 */
void start_tcp_server(t_server_conn *srv_conn,t_tcp_srv_func srv_func)
{
	struct sockaddr_in server, remote;
	int request_sock, new_sock;
	int nfound, fd, buf_len;
	//int maxfd;
	socklen_t addrlen;
	fd_set rmask, mask;
	struct timeval timeout = srv_conn->timeout;
	char buf[MAX_buf_LEN] = {0};
	int port = srv_conn->port;

	// 创建连接
	if ((request_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		srv_conn->ret = TS_ERR_PARM;
		goto error;
	}
	srv_conn->fd = request_sock;

	// 绑定端口
	memset((void *) &server, 0, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(port);
	if (bind(request_sock, (struct sockaddr *)&server, sizeof server) < 0) {
		perror("bind");
		srv_conn->ret = TS_ERR_PARM;
		goto error;
	}

	// 监听端口
	if (listen(request_sock, SOMAXCONN) < 0) {
		perror("listen");
		srv_conn->ret = TS_ERR_PARM;
		goto error;
	}

	srv_conn->ret = TS_CONN;
	srv_conn->sys_err = errno;
	srv_func.conn_status_func(srv_conn);


	FD_ZERO(&mask);
	FD_SET(request_sock, &mask);
	srv_conn->maxfd = request_sock;
	
	printf("start listen server  %d,srv sock : %d\n",port,request_sock);
	
	// 循环连接
	for (;;) {
		rmask = mask;
		nfound = select(srv_conn->maxfd + 1, &rmask, NULL, NULL, &timeout);
		//nfound = select(FD_SETSIZE, &rmask, NULL, NULL, &timeout);
		if (nfound < 0) {
			if (EINTR == errno) {
				printf("interrupted system call\n");
				continue;
			}
			// 连接出现异常
			perror("srv select ");
			//goto error;
		} else if (0 == nfound) {
			continue;
		}

		if (FD_ISSET(request_sock, &rmask)) {
			// 新的连接
			addrlen = sizeof(remote);
			new_sock = accept(request_sock, (struct sockaddr *)&remote,&addrlen);
			if (new_sock < 0) {
				perror("accept");
				goto error;
			}
			printf("connection from host %s, port %d, socket %d\n",(char *)inet_ntoa(remote.sin_addr), ntohs(remote.sin_port),new_sock);

			FD_SET(new_sock, &mask);
			if (new_sock > srv_conn->maxfd) {
				srv_conn->maxfd = new_sock;
			}
			FD_CLR(request_sock, &rmask);
		}

		for (fd = 0; fd <= srv_conn->maxfd ; fd++) {
			// 循环查询连接的数据
			if (FD_ISSET(fd, &rmask)) {
				buf_len = read(fd, buf, sizeof(buf) - 1);
				if (buf_len < 0) {
					//
					perror("read");
					goto error;
				} else if (buf_len <= 0) {
					// 连接断开
					printf("conn broken, fd:%d\n",fd);
					FD_CLR(fd, &mask);
					if (close(fd)) {
						perror("close");
						goto error;
					}
					continue;
				} else {
					t_tcp_recv_data tcp_recv_data;
					tcp_recv_data.fd = fd;
					tcp_recv_data.buf = buf;
					tcp_recv_data.buf_len = buf_len;
					srv_func.recv_data_func(srv_conn,tcp_recv_data);
				}
			} 
		} 
	} 
//错误
error:
	if (EXIT_SUCCESS == srv_conn->ret) {
		srv_conn->ret = TS_ERR_NET;
		srv_conn->sys_err = errno;
	}
	if (srv_conn->fd > 0) {
		close(srv_conn->fd);
	}
	srv_conn->ret = TS_ERR_NET;
	srv_conn->sys_err = errno;
	if (NULL != srv_func.conn_status_func) {
		srv_func.conn_status_func(srv_conn);
	}
}

/////////////////////////////////////////////////////////////////////////////////

/**
 * 连接到服务器
 */
void start_udp_client(t_client_conn *cli_conn,t_cli_func sock_func)
{
	struct sockaddr_in server_addr;//socket地址
	fd_set read_set_bak,read_set;//读取结果集
	struct hostent *host = NULL;
	char ip_str[32] = {0};
	char recv_buf[MAX_buf_LEN] = {0};
	int recv_len = 0;
	int opt = 1; 

	if (NULL == cli_conn) {
		printf("cli_conn is null\n");
		cli_conn->ret = TS_ERR_PARM;
		return;
	}

	//初始化
	cli_conn->sockfd = 0;
	cli_conn->ret = EXIT_SUCCESS;
	bzero(recv_buf,sizeof(recv_buf));

	//gethostbyname可以通过主机名称得到主机的IP地址
	host = gethostbyname((const char *)cli_conn->ip);
	if (NULL == host) {
		printf("Gethostname error\n");
		cli_conn->ret = TS_ERR_PARM;
		return;
	}
	
	// 客户程序开始建立 sockfd描述符
	cli_conn->sockfd = socket(AF_INET,SOCK_DGRAM,0);
	if (-1 == cli_conn->sockfd) {
		printf("Socket Error:%s\a\n",strerror(errno));
		goto error;
	}

	//设置该套接字为广播类型
	if (setsockopt(cli_conn->sockfd, SOL_SOCKET, SO_BROADCAST, (char *)&opt, sizeof(opt)) < 0 ) {
		printf("Can not broadcast\n");
		goto error;
	}  

	// 客户程序填充服务端的资料
	bzero(&(server_addr),sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	
	// 主机字节序转换为网络字节序
	server_addr.sin_port = htons(cli_conn->port);
	inet_ntop(host->h_addrtype, *(host->h_addr_list), ip_str, sizeof(ip_str));
	server_addr.sin_addr.s_addr = inet_addr(ip_str);
	if (server_addr.sin_addr.s_addr == INADDR_NONE){
		printf("Incorrect ip address!\n");
		goto error;
	}

	if (connect(cli_conn->sockfd,(struct sockaddr *)&server_addr,sizeof(server_addr))<0) {
		printf("Connect error\n");
		goto error;
	}

	printf("Start udp conn %s : %d\n",ip_str,cli_conn->port);
	cli_conn->ret = TS_CONN;
	if (NULL != sock_func.on_conn_status) {
		sock_func.on_conn_status(cli_conn);
	}
	
	FD_ZERO(&read_set);
	FD_SET(cli_conn->sockfd, &read_set);
	//开始接收数据
	while(1) {
		read_set_bak = read_set;	
		cli_conn->ret = select(cli_conn->sockfd + 1, &read_set_bak, NULL, NULL, &cli_conn->timeval);
		if (-1 == cli_conn->ret) {
			printf("select failed after connected %s !!! \n",strerror(errno));
			goto error;
		} else if (0 == cli_conn->ret) {
			continue;
		} else {
			if ((cli_conn->ret = FD_ISSET(cli_conn->sockfd, &read_set_bak)) > 0) { //socket中获取数据
				bzero(recv_buf,sizeof(recv_buf));
				recv_len = recv(cli_conn->sockfd , recv_buf , sizeof(recv_buf),0);
				if (0 >= recv_len) {
					printf("Read Error:%s\n",strerror(errno));
					goto error;
				} else {
					//调用回调
					if (NULL != sock_func.on_recv_data) {
						sock_func.on_recv_data(cli_conn,recv_buf,recv_len);
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
	if (EXIT_SUCCESS == cli_conn->ret) {
		cli_conn->ret = TS_ERR_NET;
		cli_conn->sys_err = errno;
	}
	if (cli_conn->sockfd > 0) {
		close(cli_conn->sockfd);
	}
	cli_conn->ret = TS_ERR_NET;
	cli_conn->sys_err = errno;
	if (NULL != sock_func.on_conn_status) {
		sock_func.on_conn_status(cli_conn);
	}

	return;
}

/**
 * 连接到UDP服务器
 */
void start_udp_server(t_server_conn *srv_conn,t_udp_srv_func srv_func)
{
	struct sockaddr_in server, client;
	int request_sock;
	int nfound, maxfd, buf_len;
	fd_set rmask, mask;
	struct timeval timeout = srv_conn->timeout;
	char buf[MAX_buf_LEN] = {0};
	int port = srv_conn->port;
	int client_len = 0;
	int opt = 1;

	//参数检测
	if (NULL == srv_conn ) {
		printf("srv conn is null\n");
		return;
	}
	if (NULL == srv_func.recv_data_func) {
		printf("recv_data_func is null");
		return;
	}
	if (NULL == srv_func.conn_status_func) {
		printf("conn_status_fun is null");
		return;
	}

	// 创建连接
	if ((request_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket");
		srv_conn->ret = TS_ERR_PARM;
		goto error;
	}
	srv_conn->fd = request_sock;
	
	//设置该套接字为广播类型
	if (setsockopt(request_sock, SOL_SOCKET, SO_BROADCAST, (char *)&opt, sizeof(opt)) < 0 ) {
		printf("Can not broadcast\n");
		goto error;
	}

	// 绑定端口
	memset((void *) &server, 0, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	server.sin_port = htons(port);
	if (bind(request_sock, (struct sockaddr *)&server, sizeof server) < 0) {
		perror("bind");
		srv_conn->ret = TS_ERR_PARM;
		goto error;
	}
	
	// 调用连接成功的回调
	srv_conn->ret = TS_CONN;
	srv_conn->sys_err = errno;
	srv_func.conn_status_func(srv_conn);

	FD_ZERO(&mask);
	FD_SET(request_sock, &mask);
	maxfd = request_sock;
	
	printf("start udp server  %d\n",port);
	client_len = sizeof(client);

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
			perror("select get");
			goto error;
			return;
		} else if (nfound == 0) {
			continue;
		} else if (FD_ISSET(request_sock, &rmask)) {
			buf_len = recvfrom(request_sock, buf, sizeof(buf),0,(struct sockaddr *)&client,(socklen_t *)&client_len);
			if (buf_len < 0) {
				//
				perror("read");
				goto error;
				return;
			} else if (buf_len<=0) {
				// 连接断开
				printf("conn brocken, %d\n",request_sock);
				FD_CLR(request_sock, &mask);
				if (close(request_sock)) {
					perror("close");
					goto error;
					return;
				}
				continue;
			} else {
				t_udp_recv_data  udp_recv_data ;
				udp_recv_data.buf = buf;
				udp_recv_data.buf_len = buf_len;
				udp_recv_data.addr = client;
				udp_recv_data.addr_len = client_len;
				srv_func.recv_data_func(srv_conn,udp_recv_data);
			}
		} 
	}

//错误
error:
	if (EXIT_SUCCESS == srv_conn->ret) {
		srv_conn->ret = TS_ERR_NET;
		srv_conn->sys_err = errno;
	}
	if (srv_conn->fd > 0) {
		close(srv_conn->fd);
	}
	srv_conn->ret = TS_ERR_NET;
	srv_conn->sys_err = errno;
	if (NULL != srv_func.conn_status_func) {
		srv_func.conn_status_func(srv_conn);
	}

}

