// client = receiver
/* UDP client in the internet domain */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define HEADER_SIZE 64
#define PACKET_SIZE 1024

void error(const char *);
int main(int argc, char *argv[])
{
   int sock, n;
   unsigned int length;
   struct sockaddr_in server, from;
   struct hostent *hp;
   char buffer[1024];
   char* filename;

   if (argc != 4) {
     printf("Usage: ./client host port filename\n");
     exit(1);
   }

   // Get requested filename
   filename = argv[3];

   sock = socket(AF_INET, SOCK_DGRAM, 0);
   if (sock < 0) error("socket");
   server.sin_family = AF_INET;

   // Localhost support
   if (strncmp(argv[1],"localhost", strlen(argv[1])) == 0)
     hp = gethostbyname("127.0.0.1");
   else
     hp = gethostbyname(argv[1]);
   if (hp == 0) error("Unknown host");

   bcopy((char *)hp->h_addr, (char *)&server.sin_addr, hp->h_length);
   server.sin_port = htons(atoi(argv[2]));
   length=sizeof(struct sockaddr_in);

   bzero(buffer,PACKET_SIZE);

   //memcpy(buffer, filename, strlen(filename) + 1);

   char header[HEADER_SIZE];
   // construct header here

   // PACKET STRUCTURE
   /*


    int type;	// 0: Initial File Request, 1: Data, 2: ACK, 3: FIN (signal for final packet)
    int seq;	// Packet sequence number
    int size;	// Data size
    int checksum; // For detecting corruption

    char* data;
   */

   memcpy(buffer, header, HEADER_SIZE);

   memcpy(buffer + HEADER_SIZE, filename, strlen(filename) + 1);


   n = sendto(sock, buffer, PACKET_SIZE, 0, (const struct sockaddr*)&server, length);
   if (n < 0) error("Sendto");
   n = recvfrom(sock,buffer,256,0,(struct sockaddr *)&from, &length);
   if (n < 0) error("recvfrom");
   write(1,"Got an ack: ",12);

   write(1,buffer,n);

   close(sock);
   return 0;
}

void error(const char *msg)
{
    perror(msg);
    exit(0);
}
