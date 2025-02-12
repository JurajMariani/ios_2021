CC = gcc
CFLAGS = -std=gnu99 -Wall -Wextra -pedantic -Werror
LFLAGS = -pthread
NAME = proj2

all: build clean

build: santa_and_helpers.o proj2.o
	$(CC) $^ -o $(NAME) $(LFLAGS)

proj2.o: proj2.c santa_and_helpers.h
	$(CC) $(CFLAGS) $< -c $(LFLAGS)

santa_and_helpers.o: santa_and_helpers.c santa_and_helpers.h
	$(CC) $(CFLAGS) $< -c

clean:
	rm -f santa_and_helpers.o
	rm -f proj2.o
