#ifndef T_SOCKET_H_
#define T_SOCKET_H_

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

#define MAX_BUFF_LEN 1024

//超时
#define TS_ERR_TIMIEOUT	1
//网络异常
#define TS_ERR_NET		2
//参数异常
#define TS_ERR_PARM		3

//状态
#define TS_CONN			4

//关闭
#define TS_CLOSE		5



//连接信息
typedef struct _sock_data {
	char ip[24];//ip地址
	int port;//端口号
	struct timeval timeval ;//超时
	int sockfd;//句柄
	int ret ;//错误类型
	int sys_err;//系统错误码
}t_sock_data;

/**
 * 收到数据的回调
 */
typedef void (*recv_data_func)(t_sock_data *sc_data,char *buff,int len);

/**
 * 连接状态的回调
 */
typedef void (*conn_status_func)(t_sock_data *sc_data);

//连接的回调
typedef struct _sock_func {
	recv_data_func on_recv_data;
	conn_status_func on_conn_status;
} t_sock_func;

void t_free(void *data);

char *hex_2_str(char *hex,int len);

/**
 * 向服务器发送数据
 */
void send2server(t_sock_data *sc_data);

/**
 * 连接到服务器
 */
void conn2server(t_sock_data *sc_data,t_sock_func sock_func);


#endif

