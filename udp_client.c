/*
 * udp_client.c
 *
 *  Created on: Feb 19, 2019
 *      Author: sramirez266
 */

#include <stdlib.h>
#include <stdio.h>

#include "udp.h"
#define MILLION 1000000

typedef struct udp_host_config_t {
	char hostIP[SMALLBUF];
	char hostPort[TINYBUF];
	unsigned pingCount;
	unsigned period;
	unsigned timeout;
} udp_host_config_t;

static void
InitUDPConfig(udp_host_config_t * config) {
	memset(config->hostIP, 0, SMALLBUF);
	memset(config->hostPort, 0, TINYBUF);
	config->pingCount = 0;
	config->period = 0;
	config->timeout = 0;
}

static void
FillConfig(udp_host_config_t * config,
		const int argc, const char **argv) {
	for (int i=1; i<argc; ++i) {
		if (!strcmp(argv[i], "--server_ip")) {
			memcpy(config->hostIP, argv[i+1], strlen(argv[i+1]));
		} else if (!strcmp(argv[i], "--server_port")) {
			memcpy(config->hostPort, argv[i+1], strlen(argv[i+1]));
		} else if (!strcmp(argv[i], "--count")) {
			config.pingCount = atoi(argv[i+1]);
		} else if (!strcmp(argv[i], "--period")) {
			config->period = atoi(argv[i+1]);
		} else if (!strcmp(argv[i], "--timeout")) {
			config->timeout = atoi(argv[i+1]);
		}
	}
}

typedef struct timestamp_t timestamp_t;
typedef struct timeval timeval;
static timestamp_t
GetTimestamp() {
	timeval now;
	gettimeofday(&now, NULL);
	return now.tv_sec + (timestamp_t)(now.tv_sec * MILLION);
}

int main(int argc, char ** argv) {
	if (argc != 6) {
		fprintf(stderr, "Usage: udp_client <server_ip> <server_port> <count> <period> <timeout>\n");
		exit(EXIT_FAILURE);
	}

	udp_host_config_t config;
	InitUDPConfig(&config);
	FillConfig(&config, argc, argv);
	int sockFd = UDPClientInit(config.hostIP, config.hostPort);
	for (unsigned i=0; i<config.pingCount; ++i) {
		timestamp_t t0 = GetTimestamp();
		// send ping
		byte buf[SMALLBUF] = {0};

		if ((size_t numbytes = sendto(sockfd, buf, SMALLBUF, 0,
		             p->ai_addr, p->ai_addrlen)) == -1) {
		        perror("talker: sendto");
		        exit(1);
		    }


		// wait for receive
		// if it takes longer than "timeout", then continue
		// if a period hasn't gone by yet, wait
		timestamp_t t1 = GetTimestamp();
		double elapsed = t1 - t0;
		if (elapsed < config.period)
			sleep((config.period - elapsed)/1000);
	}
}
