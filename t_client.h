#ifndef COMMON_DEFINE_H_
#define COMMON_DEFINE_H_

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

typedef struct _sock_data {
	char ip[24];//ip地址
	int port;//端口号
	struct timeval timeval ;//超时
	int sockfd;//句柄
	int ret ;//错误类型
	//char recv_buff[1024];//收到数据的缓存
	//int recv_byte;//收到数据的长度
}t_sock_data;

//收到数据的回调
typedef void (*recv_data_func)(t_sock_data *sc_data,char *buff,int len);

//连接状态的回调
typedef void (*conn_status_func)(t_sock_data *sc_data,char *buff,int len);

void t_free(void *data);

char *hex_2_str(char *hex,int len);

/**
 *向服务器发送数据
 */
void send2server(t_sock_data *sc_data);

/**
 * 连接到服务器
 */
void conn2server(t_sock_data *sc_data,recv_data_func recv_func);

#endif
