CFLAGS=-Wall

all: client server

client: client.o
	g++ $(CFLAGS) -o client.out client.cpp

server: server.o
	g++ $(CFLAGS) -o server.out server.cpp

clean:
	$(RM) client
	$(RM) server
	$(RM) client.o
	$(RM) server.o
