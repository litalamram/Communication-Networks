CC = gcc
OBJS = seker_server.o seker_client.o Message.o Command.o
COMP_FLAG = -std=gnu99 -Wall -g -Wextra -Werror -pedantic-errors


all: seker_server seker_client
seker_server: seker_server.o Message.o
	$(CC) seker_server.o Message.o -o seker_server
seker_client: seker_client.o Message.o Command.o
	$(CC) seker_client.o Message.o Command.o -o seker_client
seker_server.o: seker_server.c Message.h
	$(CC) $(COMP_FLAG) -c $*.c
seker_client.o: seker_client.c Message.h Command.h
	$(CC) $(COMP_FLAG) -c $*.c
Message.o: Message.h Message.c
	$(CC) $(COMP_FLAG) -c $*.c
Command.o: Command.h Command.c
	$(CC) $(COMP_FLAG) -c $*.c

clean:
	rm -f $(OBJS) seker_server seker_client
