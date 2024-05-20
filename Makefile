CC = gcc
CFLAGS = -Wall -Wextra -fsanitize=address

default: spchk

spchk: spchk.c
	$(CC) $(CFLAGS) -o spchk spchk.c

clean:
	rm -f spchk 
	clear
