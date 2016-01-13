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

/**
 * 释放
 */
void t_free(void *data);

/**
 * 打印hex
 */
char *hex_2_str(char *hex,int len);

//连接信息
typedef struct _sock_data {
	char ip[24];//ip地址
	int port;//端口号
	struct timeval timeval ;//超时
	int sockfd;//句柄
	int ret ;//错误类型
	int sys_err;//系统错误码
}t_client_conn;

/**
 * 收到数据的回调
 */
typedef void (*recv_data_func)(t_client_conn *cli_conn,char *buff,int len);

/**
 * 连接状态的回调
 */
typedef void (*conn_status_func)(t_client_conn *cli_conn);

//连接的回调
typedef struct _cli_func {
	recv_data_func on_recv_data;
	conn_status_func on_conn_status;
} t_cli_func;


/**
 * 向服务器发送数据
 */
void send2server(t_client_conn *cli_conn);

/**
 * 连接到服务器
 */
void conn2server(t_client_conn *cli_conn,t_cli_func sock_func);

/**
 * 服务器的信息
 */
typedef struct _server_data {
	int port;
	struct timeval timeval ;//超时
	int ret ;//错误类型
	int sys_err;//系统错误码
} t_server_conn;

/**
 * 收到数据的回调
 */
typedef void (*srv_recv_data_func)(t_server_conn *srv_conn,int fd,char *buff,int len);

/**
 * 连接状态的回调
 */
typedef void (*srv_conn_status_func)(t_server_conn *srv_conn);

typedef struct _srv_func {
	srv_recv_data_func recv_data_func;
	srv_conn_status_func conn_status_func;
} t_srv_func;

/**
 * 启动服务器
 */
void start_server(t_server_conn *srv_conn,t_srv_func srv_func);

#endif

