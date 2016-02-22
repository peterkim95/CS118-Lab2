CFLAGS=-Wall

all: client server

client: client.o
	gcc $(CFLAGS) -o client client.c

server: server.o
	gcc $(CFLAGS) -o server server.c

clean:
	$(RM) client
	$(RM) server
	$(RM) client.o
	$(RM) server.o
