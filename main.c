#include "t_socket.h"

/**
 *向服务器发送数据
 */
void send_to_server(t_client_conn *cli_conn) {
	if(NULL == cli_conn) {
		printf("send2server : cli_conn is null");
		return;
	}

	char input[MAX_BUFF_LEN] = {0};

	while(1) {
		bzero(input,sizeof(input));
		scanf("%s",input);
		write(cli_conn->sockfd,input,strlen(input));
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
			break;
		case TS_ERR_PARM:
			break;
		case TS_CONN://连接成功
			//发送
			printf("start send\n");
			pthread_create(&thread_send,NULL,(void *)send_to_server,(void *)cli_conn);
			break;
		default:
			break;
	}
}

/**
 * 收到服务器的数据
 */
void recv_from_server(t_client_conn *cli_conn,char *buff ,int len) {
	char *s = NULL;
	if(NULL == cli_conn) {
		return;
	}

	if(NULL == buff) {
		return;
	}

	s = hex_2_str(buff,len);
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
	cli_conn.timeval.tv_sec = 3;
	cli_conn.timeval.tv_usec = 0;
	
	//回调
	t_cli_func sock_func;
	sock_func.on_recv_data = recv_from_server;
	sock_func.on_conn_status = conn_status_change;

	//连接
	printf("--------client start--------\n");
	conn2server(&cli_conn,sock_func);
}


void srv_conn_func(t_server_conn *srv_conn) {	
	if(NULL == srv_conn) {
		return;
	}

	if(EXIT_SUCCESS != srv_conn->ret) {
		printf("ERR : %s\n" , strerror(srv_conn->sys_err));
	}
	
}

/**
 * 收到客户端的数据
 */
void recv_data_from_cli(t_server_conn *srv_conn,int fd,char *buff ,int len) {
	char *s = NULL;
	if(NULL == srv_conn) {
		return;
	}

	if(NULL == buff) {
		return;
	}

	s = hex_2_str(buff,len);
	if(NULL == s) {
		return;	
	}
	printf("recv :fd : %d, %s\n",fd , s);

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
	server_conn.timeval = timeval;
	server_conn.ret = EXIT_SUCCESS;
	
	//
	t_srv_func srv_func;
	srv_func.recv_data_func = recv_data_from_cli;
	srv_func.conn_status_func = srv_conn_func;
	
	printf("--------server start--------\n");
	start_server(&server_conn,srv_func);
}

int main(int argc, char *argv[])
{
	int ret = EXIT_SUCCESS;
	int port = 0;
	char *ip = NULL;

	if(argc == 3) {
		ip = argv[1];
		port = atoi(argv[2]);
		run_client(ip,port);
	} else if(argc == 2){
		port = atoi(argv[1]);
		run_server(port);
	}
}

