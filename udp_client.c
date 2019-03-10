/*
 * udp_client.c
 *
 *  Created on: Feb 19, 2019
 *      Author: sramirez266
 */

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <poll.h>

#include "udp.h"

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

typedef struct ping_results_t {
	unsigned successfulReplies;
	double maxElapsed;
	double minElapsed;
	double avgElapsed;
}ping_results_t;

typedef struct thread_context_t {
	udp_host_config_t * config;
	pthread_mutex_t * mutex;
	ping_results_t * results;
} thread_context_t;

static timestamp_t
GetTimestamp() {
	timeval now;
	gettimeofday(&now, NULL);
	return now.tv_sec + (timestamp_t)(now.tv_sec * MILLION);
}

void
WorkerThread(void * arg) {
	thread_context_t * context = (thread_context_t *) arg;

	udp_connection_t * conn = UDPClientInit(
			context->config.hostIP,
			context->config.hostPort);
	assert(conn->socketFd != SOCKET_CLOSED);

	// send ping
	byte buf[SMALLBUF] = {0};

	ssize_t numBytes;

	numBytes = sendto(conn->socketFd, buf, SMALLBUF, 0,
		conn->hostInfo->ai_addr, conn->hostInfo->ai_addrlen);

	if (numBytes == -1) {
		perror("WorkerThread:: sendto error");
		exit(1);
	}

	// start tracking time once the ping has been sent
	timestamp_t t0 = GetTimestamp();

	struct pollfd pollInfo;
	pollInfo.events = POLLIN | POLLERR | POLLHUP | POLLNVAL;
	pollInfo.fd = conn->socketFd;
	int ready = poll(&pollInfo, 1, context->config->timeout);
	if (pollInfo.revents==POLLIN && ready!=0) {
		numBytes = recvfrom(conn->socketFd, buf, SMALLBUF-1 , 0,
			conn->hostInfo->ai_addr, conn->hostInfo->ai_addr);

		if (numBytes == -1) {
			perror("WorkerThread:: recvfrom error\n");
			exit(1);
		}
		// time stamp
		timestamp_t t1 = GetTimestamp();
		double elapsed = t1 - t0;
		if (elapsed < context->config.timeout) {
			pthread_mutex_lock(context->mutex);
			++context->results->successfulReplies;
			context->results->maxElapsed = MAX(elapsed, context->results->maxElapsed);
			context->results->minElapsed = MIN(elapsed, context->results->minElapsed);
			context->results->avgElapsed += elapsed; // will be averaged later
			pthread_mutex_unlock(context->mutex);
		}
	} else if (ready==0) {
		printf("WorkerThread:: Timeout occured\n");
	} else {
		perror("WorkerThread:: poll error");
	}

	UDPConnectionFree(&conn);
}

int main(int argc, char ** argv) {
	if (argc != 6) {
		fprintf(stderr, "Usage: udp_client <server_ip> <server_port> <count> <period> <timeout>\n");
		exit(EXIT_FAILURE);
	}

	udp_host_config_t config;
	thread_context_t context;
	ping_results_t results;
	memset(&results, 0, sizeof(results)); // 0 out everything
	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	InitUDPConfig(&config);
	FillConfig(&config, argc, argv);
	context.config = &config;
	context.mutex = &mutex;
	context.results = &results;

	for (unsigned i=0; i<config.pingCount; ++i) {
		timestamp_t t0 = GetTimestamp();
		pthread_t worker;
		pthread_create(&worker, NULL, WorkerThread, &context);
		sleep(config.period);
		timestamp_t t1 = GetTimestamp();
		// elapsed time will effectively be 0 since
		// the code above takes no time to run
		// but just to be thorough...
		double elapsed = t1 - t0;
		if (elapsed < config.period)
			sleep((config.period - elapsed)/1000);

	}
}
