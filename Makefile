CC=gcc
DEBUG=-g3 -pedantic -Wall -Wextra -Werror
OPT=-pedantic -O3 -Wall -Werror -Wextra

ifneq (,$(filter $(MAKECMDGOALS),debug valgrind))
CFLAGS=$(DEBUG)
else
CFLAGS=$(OPT)
endif

.PHONY: clean

all: clean udp_client

debug: all
	
valgrind: clean tests 
	valgrind --leak-check=full --log-file="valgrind.out" --show-reachable=yes -v ./tests

udp_client: udp_client.o udp.o
	$(CC) $(CFLAGS) -lpthreads -o $@ $^
	
udp_client.o: udp_client.c
	$(CC) $(CFLAGS) -lpthreads -c -o $@ $<
	
udp.o: udp.c response.h
	$(CC) $(CFLAGS) -lpthreads -c -o $@ $<
	
clean:
	rm -rf *.o *.exe
	find . -maxdepth 1 -type f -executable -delete