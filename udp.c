/*
 * udp.c
 *
 *  Created on: Feb 19, 2019
 *      Author: sramirez266
 */


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

// Client side UDP connection initiialization.
// Constructs a socket. Does not call bind or connect
// since this is UDP. returns the udp connection object
// which holds info for the main program
udp_connection_t *
UDPConnectionInit(const char * hostIp, const char * hostPort) {
	(void)hostIp;
	int status = STATUS_BAD;
	int sockFd = SOCKET_CLOSED;
	addrinfo hints;
	addrinfo * hostInfo;
	addrinfo * cur;
	CheckValidPort(hostPort);
	memset(&hints, 0, sizeof(hints)); // make sure the struct is empty
	hints.ai_family = AF_UNSPEC;     // don't care IPv4 or IPv6
	hints.ai_socktype = SOCK_DGRAM; // UDP sockets
	hints.ai_flags = AI_PASSIVE;	// use this IP
	hints.ai_protocol = 0;
	hints.ai_canonname = NULL;
	hints.ai_next = NULL;

	// get ready to connect
	if ((status = getaddrinfo(hostIp, hostPort, &hints, &hostInfo)) != 0) {
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
		exit(EXIT_FAILURE);
	}

	for (cur=hostInfo; cur!=NULL; cur=cur->ai_next) {
		sockFd = socket(cur->ai_family,	cur->ai_socktype, cur->ai_protocol);
		if (sockFd == -1) {
			perror("socket");
			continue; // unsuccessful address structure
		}
		break; // successful connection
	}

	if (cur==NULL) {
		fprintf(stderr, "Could not create socket\n");
		exit(EXIT_FAILURE);
	}

	udp_connection_t * ret = malloc(sizeof(udp_connection_t));
	ret->hostInfo = hostInfo;
	ret->socketFd = sockFd;
	return ret;
}

void
UDPConnectionFree(udp_connection_t * conn) {
	freeaddrinfo(conn->hostInfo);
	if (conn->socketFd != SOCKET_CLOSED)
		close(conn->socketFd);
	free(conn);
}
