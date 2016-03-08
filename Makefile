CFLAGS=-Wall

all: client server

client: client.o
	g++ $(CFLAGS) -o client client.cpp

server: server.o
	g++ $(CFLAGS) -o server server.cpp

clean:
	$(RM) client
	$(RM) server
	$(RM) client.o
	$(RM) server.o
