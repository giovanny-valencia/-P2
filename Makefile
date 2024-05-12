CC = gcc
CFLAGS = -Wall -Wextra -g -fsanitize-undefined-trap-on-error


default: spchk

spchk: spchk.c
	$(CC) $(CFLAGS) -o spchk spchk.c

clean:
	del spchk.exe
ifdef OS
	cls
else
	clear
endif
