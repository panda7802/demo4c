#include "t_socket.h"

/**
 *向服务器发送数据
 */
void send_to_server(t_sock_data *sc_data) {
	if(NULL == sc_data) {
		printf("send2server : sc_data is null");
		return;
	}

	char input[MAX_BUFF_LEN] = {0};

	while(1) {
		bzero(input,sizeof(input));
		scanf("%s",input);
		write(sc_data->sockfd,input,strlen(input));
		usleep(100);
		if(0 == strcmp(input,"q")) {
			close(sc_data->sockfd);
		}
	}
}

/**
 *
 */
void conn_status_change(t_sock_data *sc_data) {
	pthread_t thread_send;
	
	if (NULL == sc_data) {
		return;
	}

	switch(sc_data->ret) {
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
			pthread_create(&thread_send,NULL,(void *)send_to_server,(void *)sc_data);
			break;
		default:
			break;
	}
}

/**
 * 收到服务器的数据
 */
void recv_from_server(t_sock_data *sc_data,char *buff ,int len) {
	char *s = NULL;
	if(NULL == sc_data) {
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

int main(int argc, char *argv[])
{
	int ret = EXIT_SUCCESS;

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
	
	//回调
	t_sock_func sock_func;
	sock_func.on_recv_data = recv_from_server;
	sock_func.on_conn_status = conn_status_change;

	//连接
	conn2server(&sc_data,sock_func);

}

