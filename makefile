#Declare the variable
CC=gcc
CFLAGS=-o

#Declare commands

#Default command
all:
	$(CC) cshell.c $(CFLAGS) cshell.out

clean:
	rm -rf cshell.out