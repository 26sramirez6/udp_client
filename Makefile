CC=gcc
DEBUG=-g3 -pedantic -Wall -Wextra -Werror
OPT=-pedantic -O3 -Wall -Werror -Wextra -D NO_CLIENT_DEBUG

ifneq (,$(filter $(MAKECMDGOALS),debug valgrind))
CFLAGS=$(DEBUG)
else
CFLAGS=$(OPT)
endif

.PHONY: clean

all: clean udp_client

debug: all
	
valgrind: clean tests 
	valgrind --leak-check=full --log-file="valgrind.out" --show-reachable=yes -v ./udp_client --server_ip=localhost --server_port=

udp_client: udp.o udp_client.o
	$(CC) $(CFLAGS) -o $@ $^ -lpthread
	
udp_client.o: udp_client.c udp.h
	$(CC) $(CFLAGS) -c -o $@ $<
	
udp.o: udp.c udp.h
	$(CC) $(CFLAGS) -c -o $@ $<
	
clean:
	rm -rf *.o *.exe
	find . -maxdepth 1 -type f -executable -delete
