CC=gcc
DEBUG=-g3 -pedantic -Wall -Wextra -Werror
OPT=-pedantic -O3 -Wall -Werror -Wextra

ifneq (,$(filter $(MAKECMDGOALS),debug valgrind))
CFLAGS=$(DEBUG)
else
CFLAGS=$(OPT)
endif

.PHONY: clean

all: clean web_server

debug: all
	
valgrind: clean tests 
	valgrind --leak-check=full --log-file="valgrind.out" --show-reachable=yes -v ./tests
	
web_server: response.o web_server.o util.o vararray.o
	$(CC) $(CFLAGS) -o $@ $^ -lpthread
	
web_server.o: web_server.c util.h response.h vararray.h
	$(CC) $(CFLAGS) -c -o $@ $<
	
tests: tests.o response.o vararray.o util.o
	$(CC) $(CFLAGS) -o $@ $^
	
tests.o: tests.c
	$(CC) $(CFLAGS) -c -o $@ $<

response.o: response.c response.h
	$(CC) $(CFLAGS) -c -o $@ $<

vararray.o: vararray.c vararray.h
	$(CC) $(CFLAGS) -c -o $@ $<

util.o: util.c util.h
	$(CC) $(CFLAGS) -c -o $@ $<
	
clean:
	rm -rf *.o *.exe
	find . -maxdepth 1 -type f -executable -delete