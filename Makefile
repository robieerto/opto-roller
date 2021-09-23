CC=g++
CFLAGS=-std=c++11 -Wall -Wextra -g

.PHONY: all clean

all: program
program:
	$(CC) $(CFLAGS) main.cpp -o main

clean:
	rm -f main
