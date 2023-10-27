CCFLAGS = -Wall -Werror -g
LDFDLAGS = -lpthread
CC = gcc

all: webserver

webserver: webserver.o socket.o thread.o
	$(CC) $(CCFLAGS) -o webserver webserver.o socket.o thread.o $(LDFDLAGS)

webserver.o: webserver.c
	$(CC) $(CCFLAGS) -c webserver.c

socket.o: socket.c
	$(CC) $(CCFLAGS) -c socket.c

thread.o: thread.c
	$(CC) $(CCFLAGS) -c thread.c

clean:
	rm -f *.o webserver