CFLAGS = -Wall -Wextra -Wunused -pedantic

all: main

main: main.c
	cc $(CFLAGS) -o withenv main.c
