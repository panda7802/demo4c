#include "t_socket.h"
#include <pthread.h>
#include <arpa/inet.h>

#define NET_TYPE_TCP 't'
#define NET_TYPE_UDP 'u'

char type = NET_TYPE_TCP;
int curr_fd = -1;

/**
 *向服务器发送数据
 */
void send_to_server(t_client_conn *cli_conn) {
	if(NULL == cli_conn) {
		printf("send2server : cli_conn is null");
		return;
	}

	char input[MAX_buf_LEN] = {0};

	while(1) {
		bzero(input,sizeof(input));
		scanf("%s",input);
		if(NET_TYPE_TCP == type) {
			write(cli_conn->sockfd,input,strlen(input));
		} else {
			send(cli_conn->sockfd,input,strlen(input) ,0);
		}
		usleep(100);
		if(0 == strcmp(input,"q")) {
			close(cli_conn->sockfd);
		}
	}
}

/**
 * 连接状态变化
 */
void conn_status_change(t_client_conn *cli_conn) {
	pthread_t thread_send;
	
	if (NULL == cli_conn) {
		return;
	}

	switch(cli_conn->ret) {
		case TS_ERR_TIMIEOUT:
			printf("---RECONNECT---\n");
			break;
		case TS_ERR_NET:
			printf("err : TS_ERR_NET\n");
			break;
		case TS_ERR_PARM:
			printf("err : TS_ERR_PARM\n");
			break;
		case TS_CONN://连接成功
			//发送
			printf("start send\n");
			curr_fd = cli_conn->sockfd;
			pthread_create(&thread_send,NULL,(void *)send_to_server,(void *)cli_conn);
			break;
		default:
			break;
	}
}

/**
 * 收到服务器的数据
 */
void recv_from_server(t_client_conn *cli_conn,char *buf ,int len) {
	char *s = NULL;
	if(NULL == cli_conn) {
		return;
	}

	if(NULL == buf) {
		return;
	}

	s = hex_2_str(buf,len);
	if(NULL == s) {
		return;	
	}

	printf("recv : %s\n",s);

	t_free(s);
}

void run_client(char *ip,int port) {
	struct timeval timeval = {0};
	timeval.tv_sec = 3;
	timeval.tv_usec = 0;
	t_client_conn cli_conn;

	//ip 端口
	bzero(cli_conn.ip,sizeof(cli_conn.ip));
	strcpy(cli_conn.ip,ip);
	cli_conn.port = port;

	//设置超时
	cli_conn.timeval = timeval;
	
	//回调
	t_cli_func sock_func;
	sock_func.on_recv_data = recv_from_server;
	sock_func.on_conn_status = conn_status_change;

	//连接
	printf("--------client start type %c--------\n",type);
	if(NET_TYPE_TCP == type) {
		start_tcp_client(&cli_conn,sock_func);
	} else {
		start_udp_client(&cli_conn,sock_func);
	}
}


void srv_conn_func(t_server_conn *srv_conn) {	
	if(NULL == srv_conn) {
		return;
	}

	switch(srv_conn->ret) {
		case TS_ERR_TIMIEOUT:
			printf("---RECONNECT---\n");
			break;
		case TS_ERR_NET:
			printf("srv TS_ERR_NET\n");
			break;
		case TS_ERR_PARM:
			printf("srv TS_ERR_PARM\n");
			break;
		case TS_CONN://连接成功
			//发送
			printf("cb start server success\n");
			curr_fd = srv_conn->fd;
			break;
		default:
			break;
	}

}

/**
 * 收到客户端的数据
 */
void tcp_recv_data_from_cli(t_server_conn *srv_conn,t_tcp_recv_data tcp_recv_data) {
	char *s = NULL;
	struct sockaddr_in sa;
	int sock_len = sizeof(sa);
	char s2send[MAX_buf_LEN] = {0};
	
	if(NULL == srv_conn) {
		return;
	}

	if(NULL == tcp_recv_data.buf) {
		return;
	}

	s = hex_2_str(tcp_recv_data.buf,tcp_recv_data.buf_len);
	if(NULL == s) {
		return;	
	}
	if(!getpeername(tcp_recv_data.fd, (struct sockaddr *)&sa, (socklen_t *)&sock_len)) {
		printf("client ip : %s ", (char *)inet_ntoa(sa.sin_addr));
		printf("client port :%d ", ntohs(sa.sin_port));
	}
	printf("recv :fd : %d, %s\n",tcp_recv_data.fd , s);
	strcpy(s2send,"send");
	write(tcp_recv_data.fd,s2send,strlen(s2send));

	if ((1 == tcp_recv_data.buf_len) && (tcp_recv_data.buf[0] == 'q')) {
		printf("I will close %d\n,",tcp_recv_data.fd);
		close(tcp_recv_data.fd);
	}

	t_free(s);
}

/**
 * 收到客户端的数据
 */
void udp_recv_data_from_cli(t_server_conn *srv_conn,t_udp_recv_data recv_data) {
	char *s = NULL;
	struct sockaddr_in sa;
	char s2send[MAX_buf_LEN] = {0};

	if(NULL == srv_conn) {
		return;
	}

	if(NULL == recv_data.buf) {
		return;
	}

	s = hex_2_str(recv_data.buf,recv_data.buf_len);
	if(NULL == s) {
		return;	
	}

	sa = recv_data.addr;
	printf("client ip : %s ", (char *)inet_ntoa(sa.sin_addr));
	printf("client port :%d ", ntohs(sa.sin_port));
	printf("recv : %s\n",s);
	strcpy(s2send,"send");
	printf("fd : %d\n",curr_fd);
	sendto(curr_fd,s2send,strlen(s2send),0,(const struct sockaddr *)&sa,sizeof(sa));

	t_free(s);
}

/**
 * 运行服务器
 */
void run_server(int port) {

	struct timeval timeval = {0};
	timeval.tv_sec = 3;
	timeval.tv_usec = 0;

	t_server_conn server_conn;
	server_conn.port = port;
	server_conn.timeout = timeval;
	server_conn.ret = EXIT_SUCCESS;
	
	//
	
	printf("--------server start--------\n");
	
	if (NET_TYPE_UDP == type) {
		t_udp_srv_func srv_func;
		srv_func.recv_data_func = udp_recv_data_from_cli;
		srv_func.conn_status_func = srv_conn_func;
		start_udp_server(&server_conn,srv_func);
	} else {
		t_tcp_srv_func srv_func;
		srv_func.recv_data_func = tcp_recv_data_from_cli;
		srv_func.conn_status_func = srv_conn_func;
		start_tcp_server(&server_conn,srv_func);
	}
}

int main(int argc, char *argv[])
{
	int ret = EXIT_SUCCESS;
	int port = 0;
	char *ip = NULL;

	if(argc == 4) {
		type = argv[3][0];
		ip = argv[1];
		port = atoi(argv[2]);
		run_client(ip,port);
	} else if(argc == 3){
		type = argv[2][0];
		port = atoi(argv[1]);
		run_server(port);
	} else {
		printf("Tcp client : ip port t\n");
		printf("Tcp server : port t\n");
		printf("Udp client : ip port u\n");
		printf("Udp server : port u\n");
		ret = EXIT_FAILURE;
		return ret;
	}

	return ret;
}

