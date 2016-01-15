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

#define MAX_buf_LEN 1024

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
 * 连接状态的回调
 */
typedef void (*conn_status_func)(t_client_conn *cli_conn);

//////////////////////////////////////////

/**
 * 客户端收到数据的回调
 */
typedef void (*tcp_recv_data_func)(t_client_conn *cli_conn,char *buf,int len);

//连接的回调
typedef struct _cli_func {
	tcp_recv_data_func on_recv_data;
	conn_status_func on_conn_status;
} t_cli_func;

/**
 * 连接到TCP服务器
 */
void start_tcp_client(t_client_conn *cli_conn,t_cli_func sock_func);


/**
 * 服务器的信息
 */
typedef struct _server_conn {
	int port;// 端口
	struct timeval timeout ;//超时
	int ret ;//错误类型
	int sys_err;//系统错误码
	int fd;//套接字
} t_server_conn;


/**
 * TCP收到数据
 */
typedef struct _tcp_recv_data {
	int fd;
	char *buf;
	int buf_len;
} t_tcp_recv_data;

/**
 * TCP收到数据的回调
 */
typedef void (*srv_recv_data_func)(t_server_conn *srv_conn,t_tcp_recv_data recv_data);

/**
 * 连接状态的回调
 */
typedef void (*srv_conn_status_func)(t_server_conn *srv_conn);

/**
 * TCP服务器回调
 */
typedef struct _tcp_srv_func {
	srv_recv_data_func recv_data_func;
	srv_conn_status_func conn_status_func;
} t_tcp_srv_func;

/**
 * 启动TCP服务器
 */
void start_tcp_server(t_server_conn *srv_conn,t_tcp_srv_func srv_func);

//////////////////////////////////////////////////
/**
 * UDP收到数据
 */
typedef struct _udp_recv_data {
	struct sockaddr_in addr;
	int addr_len;
	char *buf;
	int buf_len;
} t_udp_recv_data;

/**
 * UDP收到数据的回调
 */
typedef void (*udp_srv_recv_data_func)(t_server_conn *srv_conn,t_udp_recv_data recv_data);


/**
 * UDP服务器回调
 */
typedef struct _udp_srv_func {
	udp_srv_recv_data_func recv_data_func;
	srv_conn_status_func conn_status_func;
} t_udp_srv_func;

/**
 * UDP客户端
 */
void start_udp_client(t_client_conn *cli_conn,t_cli_func sock_func);

/**
 * 启动UDP服务器
 */
void start_udp_server(t_server_conn *srv_conn,t_udp_srv_func srv_func);

#endif

