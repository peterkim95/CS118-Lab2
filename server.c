// sender - server
/* Creates a datagram server.  The port
   number is passed as an argument.  This
   server runs forever */

#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>

#define HEADER_SIZE 64
#define PACKET_SIZE 1024

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[])
{
   int sock, length, n;
   socklen_t fromlen;
   struct sockaddr_in server;
   struct sockaddr_in from;
   char buf[1024];

   if (argc < 2) {
      fprintf(stderr, "ERROR, no port provided\n");
      exit(0);
   }

   sock=socket(AF_INET, SOCK_DGRAM, 0);
   if (sock < 0) error("Opening socket");
   length = sizeof(server);
   bzero(&server,length);
   server.sin_family=AF_INET;
   server.sin_addr.s_addr=INADDR_ANY;
   server.sin_port=htons(atoi(argv[1]));
   if (bind(sock,(struct sockaddr *)&server,length)<0)
       error("binding");
   fromlen = sizeof(struct sockaddr_in);

   while (1) {
       n = recvfrom(sock,buf,1024,0,(struct sockaddr *)&from,&fromlen);

       if (n < 0) error("recvfrom");

       write(1,"Received a datagram: ",21);

       // Parse packet into header and data
       // char header[HEADER_SIZE];

       char* filename;
       filename = buf+HEADER_SIZE;

       printf("filename=%s\n", buf+HEADER_SIZE);

       // Check if file exists
       FILE *fp = fopen(filename, "r");
       if (!fp) error("Requested file doesn't exist in the working directory");

       // Figure out file size
       fseek(fp, 0L, SEEK_END);
       int fsize = (int) ftell(fp);
       fseek(fp, 0L, SEEK_SET);

       printf("file size = %d\n", fsize);

       int total;
    	 total = fsize / PACKET_SIZE;
    	 if (fsize % PACKET_SIZE > 0)
    	   total++;
    	 printf("Required packets: %d\n", total);

       int wsize = 5;
       int wstart = 0;

       int cur = 0;

       while (cur < total)
       {
         int wend = wstart + wsize;
         while (cur < wend)
         {
           send cur;
           cur++;

         }
       }

       // Divide file into packets

       // Send each packet as they are ready

       //write(1, buf + HEADER_SIZE, PACKET_SIZE - HEADER_SIZE);
       // printf ("DATA: %s", buf + HEADER_SIZE);
       //write(1,buf+HEADER_SIZE,n);


       n = sendto(sock,"Got your message\n",17, 0,(struct sockaddr *)&from,fromlen);
       if (n  < 0) error("sendto");
   }

   return 0;
}
