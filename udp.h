/*
 * udp.h
 *
 *  Created on: Feb 19, 2019
 *      Author: sramirez266
 */

#ifndef UDP_H_
#define UDP_H_


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <limits.h>
#include <unistd.h>

#define TINYBUF 16
#define SMALLBUF 256
#define MIDBUF 512
#define MILLION 1000000
#define SOCKET_CLOSED -1
#define STATUS_BAD -1
#define MAX(a, b) a > b ? a : b
#define MIN(a, b) a < b ? a : b

typedef unsigned char byte;
typedef struct addrinfo addrinfo;
typedef struct sockaddr_storage sockaddr_storage;
typedef struct sockaddr sockaddr;
typedef struct thread_context_t thread_context_t;
typedef struct timestamp_t timestamp_t;
typedef struct timeval timeval;


typedef struct udp_connection_t {
	int socketFd;
	addrinfo * hostInfo;
} udp_connection_t;

void
UDPBroadcast();

udp_connection_t *
UDPClientInit(const char * hostIp, const char * hostPort);

void
UDPConnectionFree(udp_connection_t * conn);
#endif /* UDP_H_ */
