/*
 * udp.h
 *
 *  Created on: Feb 19, 2019
 *      Author: sramirez266
 */

#ifndef UDP_H_
#define UDP_H_

#define SMALLBUF 128
#define TINYBUF 16

typedef unsigned char byte;
typedef struct addrinfo addrinfo;
typedef struct sockaddr_storage sockaddr_storage;
typedef struct sockaddr sockaddr;

typedef struct udp_connection_t {
	int socketFd;
	addrinfo * hostInfo;
};

void
UDPBroadcast();

int
UDPClientInit(const char * hostIp, const char * hostPort);

#endif /* UDP_H_ */
