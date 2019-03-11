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
#include <limits.h>
#include "udp.h"

// holds the general configuration
// for this udp_client, filled
// from command line args
typedef struct udp_host_config_t {
	char hostIP[SMALLBUF];
	char hostPort[TINYBUF];
	unsigned long long pingCount;
	unsigned long long period;
	unsigned long long timeout;
} udp_host_config_t;

// holds the results of the
// ping tests
typedef struct ping_results_t {
	unsigned long long successfulReplies;
	unsigned long long maxElapsed;
	unsigned long long minElapsed;
	unsigned long long avgElapsed;
	unsigned long long totalElapsed;
}ping_results_t;

// holds arguments passed into each
// thread so they can perform the necessary duties
typedef struct thread_context_t {
	udp_host_config_t * config;
	pthread_mutex_t * mutex;
	ping_results_t * results;
	unsigned sequenceId;
} thread_context_t;

// for convenience
typedef long long unsigned llu;
typedef struct thread_context_t thread_context_t;
typedef struct timeval timeval;

// init the config object
static void
InitUDPConfig(udp_host_config_t * config) {
	memset(config->hostIP, 0, SMALLBUF);
	memset(config->hostPort, 0, TINYBUF);
	config->pingCount = 0;
	config->period = 0;
	config->timeout = 0;
}

// utility string functions for command line parsing
static unsigned
GetTokenCount(const char * str, const char * delimiter) {
	char * strCopy = strdup(str);
	unsigned tokenCount = 0;
	char * token = strtok(strCopy, delimiter);
	while (token != NULL)
	{
		token = strtok(NULL, delimiter);
		tokenCount++;
	}
	free(strCopy);
	return tokenCount;
}
// utility string functions for command line parsing
static char **
StringSplit(const char * str, const char * delimiter, unsigned * n) {
	unsigned tokenCount = GetTokenCount(str, delimiter);
	char ** ret = (char **)malloc(sizeof(char *)*tokenCount);
	char * strCopy = strdup(str);
	char * token = strtok(strCopy, delimiter);
	for (unsigned i=0; i<tokenCount; i++)
	{
		ret[i] = token;
		token = strtok(NULL, delimiter);
	}
	*n = tokenCount;
	return ret;
}

// parse command line args and fill up the config object
static void
FillConfig(udp_host_config_t * config,
		const int argc, const char **argv) {
	for (int i=1; i<argc; ++i) {
		unsigned tokens = 0;
		char ** split = StringSplit(argv[i], "=", &tokens);
		assert(tokens==2);
		if (!strcmp(split[0], "--server_ip")) {
			memcpy(config->hostIP, split[1], strlen(split[1]));
		} else if (!strcmp(split[0], "--server_port")) {
			memcpy(config->hostPort, split[1], strlen(split[1]));
		} else if (!strcmp(split[0], "--count")) {
			config->pingCount = atoi(split[1]);
		} else if (!strcmp(split[0], "--period")) {
			config->period = atoi(split[1]);
		} else if (!strcmp(split[0], "--timeout")) {
			config->timeout = atoi(split[1]);
		}
		free(split[0]);
		free(split);
	}
#ifndef NO_CLIENT_DEBUG
	printf("hostIP: %s, hostPort: %s, "
			"pingCount: %lld, period: %lld, timeout: %lld\n",
			config->hostIP, config->hostPort, config->pingCount,
			config->period, config->timeout);
#endif
}

// simple millisecond calculator
static long long
GetMilliSeconds() {
	timeval now;
	gettimeofday(&now, NULL);
	return now.tv_sec*1000 + now.tv_usec/1000;
}

// get sockaddr, IPv4 or IPv6, from Beej's tutorial guide.
// used to get the sender IP address from recvfrom() call in WorkerThread()
// https://beej.us/guide/bgnet/html/multi/clientserver.html#datagram:
static void *
get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// Each thread constructs it's own UDP socket,
// sends data to the server through it,
// waits to receive data from the server
// and performs clean up. The only shared
// resource is the results struct within
// the thread_context_t. A lock is used
// to control the critical section and prevent race conditions
// while updating the ping result and max/min elapsed time.
static void *
WorkerThread(void * arg) {
	thread_context_t * context = (thread_context_t *) arg;

	udp_connection_t * conn = UDPConnectionInit(
			context->config->hostIP,
			context->config->hostPort);
	assert(conn->socketFd != SOCKET_CLOSED);
	// buf contains the ping message
	byte buf[SMALLBUF] = {0};
	const int n = snprintf((char *)buf, SMALLBUF,
		"PING %d %lld\r\n", context->sequenceId,
		GetMilliSeconds());
	assert(n > 0);

	// send the ping message (buf)
	ssize_t numBytes;
	numBytes = sendto(conn->socketFd, buf, SMALLBUF, 0,
		conn->hostInfo->ai_addr, conn->hostInfo->ai_addrlen);

	if (numBytes == -1) {
		perror("WorkerThread:: sendto error");
		exit(1);
	}

	// start tracking time since ping has been sent
	unsigned long long t0 = GetMilliSeconds();
	// construct a poll for when data in the socket is ready
	// the timeout will obviously be timeout specified in config
	struct pollfd pollInfo[1];
	pollInfo[0].events = POLLIN | POLLPRI;
	pollInfo[0].fd = conn->socketFd;
	int rv = poll(pollInfo, 1, context->config->timeout);
	if (rv>0) { // an event occured in acceptable timeframe
		switch (pollInfo[0].revents) {
		case POLLIN:; // data is ready be read, note extra ";" to allow declaration on next line
			struct sockaddr_storage hostAddr;
			socklen_t hostAddrLen = sizeof(hostAddr);
			char senderIP[INET6_ADDRSTRLEN];
			numBytes = recvfrom(conn->socketFd, buf, SMALLBUF-1 , 0,
					(struct sockaddr *)&hostAddr, &hostAddrLen);

#ifndef NO_CLIENT_DEBUG
			printf("Thread id %d received %zd bytes: %s\n",
					context->sequenceId, numBytes, buf);
#endif
			if (numBytes == -1) {
				perror("WorkerThread:: recvfrom error\n");
				exit(1);
			}
			unsigned long long t1 = GetMilliSeconds();
			unsigned long long elapsed = t1 - t0;
			// ************ENTERING CRITICAL SECTION, LOCK HERE************
			// ************ENTERING CRITICAL SECTION, LOCK HERE************
			// Note: context->results is the only shared resource
			pthread_mutex_lock(context->mutex);
			++context->results->successfulReplies;
			context->results->maxElapsed = MAX(elapsed, context->results->maxElapsed);
			context->results->minElapsed = MIN(elapsed, context->results->minElapsed);
			context->results->avgElapsed += elapsed; // will be averaged later

			printf("PONG %s: seq=%d time=%lld ms\n",
				inet_ntop(hostAddr.ss_family,
						  get_in_addr((struct sockaddr *)&hostAddr),
						  senderIP, sizeof(senderIP)),
				context->sequenceId,
				elapsed);
			pthread_mutex_unlock(context->mutex);
			// ************EXITING CRITICAL SECTION, UNLOCK HERE************
			// ************EXITING CRITICAL SECTION, UNLOCK HERE************
			break;
#ifndef NO_CLIENT_DEBUG
		case POLLPRI:
			printf("WorkerThread:: Exceptional conditions on file descriptor\n");
			break;
		case POLLERR:
			printf("WorkerThread:: Error conditions on file descriptor\n");
			break;
		case POLLHUP:
			printf("WorkerThread:: File descriptor valid, but no longer connected\n");
			break;
		case POLLNVAL:
			printf("WorkerThread:: Invalid poll request, socket not open\n");
			break;
#endif
		}

	} else if (rv==0) { // timeout occured
#ifndef NO_CLIENT_DEBUG
		printf("WorkerThread:: Timeout\n");
#endif
	} else { // poll error
		perror("WorkerThread:: poll error");
	}

	UDPConnectionFree(conn);
	return NULL;
}

// print the final ping statistics
static void
PrintStats(udp_host_config_t * config, ping_results_t * results) {
	float loss = (1 - (float)results->successfulReplies/(float)config->pingCount)*100;
	llu avg = results->successfulReplies == 0 ? 0 : results->avgElapsed / results->successfulReplies;
	results->minElapsed = results->successfulReplies == 0 ? 0 : results->minElapsed;
	printf("\n--- %s ping statistics ---\n"
		"%lld transmitted, %lld received, %d%% loss, time %lld ms\n"
		"rtt min/avg/max = %lld/%lld/%lld ms\n",
		config->hostIP, config->pingCount, results->successfulReplies, (int)loss,
		results->totalElapsed, results->minElapsed, avg, results->maxElapsed);
}

int
main(int argc, char ** argv) {
	if (argc != 6) {
		fprintf(stderr, "Usage: udp_client <server_ip> "
			"<server_port> <count> <period> <timeout>\n");
		exit(EXIT_FAILURE);
	}

	// the config struct holds the command line arguments
	// initialize it and fill it up
	udp_host_config_t config;
	InitUDPConfig(&config);
	FillConfig(&config, argc, (const char **)argv);
	// the ping results struct holds the max/min/avg elapsed time
	// along with the number of successful ping replies etc.
	// Note this will be a shared resource among threads
	ping_results_t results;
	memset(&results, 0, sizeof(results)); // 0 out everything
	results.minElapsed = LLONG_MAX; // except the min elapsed..


	// each ping count will be encapsulated in
	// it's own thread. thus, construct a thread
	// context for each ping count
	thread_context_t * contexts = malloc(config.pingCount*sizeof(thread_context_t));
	// 1 thread per ping count
	pthread_t * workers = malloc(config.pingCount*sizeof(pthread_t));
	// we need 1 mutex for locking the critical section
	// in WorkerThread() for updating ping results.
	// the address of this 1 mutex will be stored in
	// each thread_context struct.
	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;


	printf("PING %s\n", config.hostIP);
	// start the timer for calculating total
	// elapsed time
	unsigned long long t0 = GetMilliSeconds();

	// for each ping count, create a thread
	// and call WorkerThread() which will
	// construct a socket, send data, and poll
	// the socket for read data from server
	for (unsigned i=0; i<config.pingCount; ++i) {
		contexts[i].config = &config;
		contexts[i].results = &results;
		contexts[i].mutex = &mutex;
		contexts[i].sequenceId = i;
		pthread_create(&workers[i], NULL, WorkerThread, &contexts[i]);

		// sleep for the period
		struct timespec sleepTime;
		sleepTime.tv_sec = config.period/1000;
		sleepTime.tv_nsec = (config.period % 1000)*1000;
		nanosleep(&sleepTime, NULL);
	}
	// join the threads here
	for (unsigned i=0; i<config.pingCount; ++i) {
		pthread_join(workers[i], NULL);
	}
	// stop the timer for total elapsed time
	unsigned long long t1 = GetMilliSeconds();
	results.totalElapsed = t1 - t0;
	// print the final stats and we are done
	PrintStats(&config, &results);
	free(workers);
	free(contexts);
}
