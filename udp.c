/*
 * udp.c
 *
 *  Created on: Feb 19, 2019
 *      Author: sramirez266
 */


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

#include "udp.h"

// simple checks on validity of port string passed
static long int
CheckValidPort(const char * str) {
	char * endptr;
	long int val;
	val = strtol(str, &endptr, 10);
	if ((errno == ERANGE && (val == INT_MAX || val == INT_MIN))
		   || (errno != 0 && val == 0)) {
	   fprintf(stderr, "invalid port number: '%s'\n", str);
	   exit(EXIT_FAILURE);
	} else if (endptr == str) {
		fprintf(stderr, "No digits were found\n");
		exit(EXIT_FAILURE);
	} else if (val < 1024) {
		fprintf(stderr, "port numbers below 1024 reserved: '%s'\n", str);
		exit(EXIT_FAILURE);
	}
	return val;
}

int
UDPClientInit(const char * hostIp, const char * hostPort) {
	int status, sockFd;
	addrinfo hints;
	addrinfo * hostInfo;
	addrinfo * cur;
	CheckValidPort(hostPort);
	memset(&hints, 0, sizeof hints); // make sure the struct is empty
	hints.ai_family = AF_UNSPEC;     // don't care IPv4 or IPv6
	hints.ai_socktype = SOCK_DGRAM; // UDP sockets
	hints.ai_flags = AI_PASSIVE;
	hints.ai_protocol = 0;
	hints.ai_canonname = NULL;
	hints.ai_next = NULL;

	// get ready to connect
	if ((status = getaddrinfo(hostIp, hostPort, &hints, &hostInfo)) != 0) {
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
		exit(EXIT_FAILURE);
	}

	// let the kernel choose the local port with connect() instead of bind()
	for (cur=hostInfo; cur!=NULL; cur=cur->ai_next) {
		sockFd = socket(cur->ai_family,	cur->ai_socktype, cur->ai_protocol);
		if (sockFd == -1) {
			continue; // unsuccessful address structure
		} else if (connect(sockFd, cur->ai_addr, cur->ai_addrlen)==0) {
			break; // successful connection
		}
		close(sockFd);
	}

	if (cur==NULL) {
		fprintf(stderr, "Could not connect socket\n");
		exit(EXIT_FAILURE);
	}

	udp_connection_t ret;


	freeaddrinfo(hostInfo);

	return sockFd;
}

void
UDPClientSend(int sockFd) {

}
