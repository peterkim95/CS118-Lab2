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
#include <fcntl.h>

#include "packet.c"

int main(int argc, char *argv[])
{
  int sock, length, n;
  socklen_t clientlen;
  struct sockaddr_in server, client;



  struct packet incoming, outgoing;

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
  if (bind(sock, (struct sockaddr *)&server, length) < 0)
    error("binding");
  clientlen = sizeof(struct sockaddr_in);

  while (1)
  {

    int flags = fcntl(sock, F_GETFL, 0); // get the flags currently set for the socket
    fcntl( sock, F_SETFL, flags | O_NONBLOCK );  // make the socket nonblocking

    // Listen until we get a file request
    if (recvfrom(sock,&incoming, sizeof(incoming),0,(struct sockaddr *)&client,&clientlen) < 0) {
      sleep(1);
      continue;
    }

    write(1,"Received a datagram: ",21);



    char* filename;
    filename = incoming.data;

    printf("filename=%s\n", filename);

    // Check if file exists
    FILE *fp = fopen(filename, "r");
    if (!fp) error("ERROR - Requested file doesn't exist in the working directory!");

    // Figure out file size
    fseek(fp, 0L, SEEK_END);
    int fsize = (int) ftell(fp);
    fseek(fp, 0L, SEEK_SET);

    printf("file size = %d\n", fsize);

    int total;
    total = fsize / 1024;
    if (fsize % 1024 > 0)
      total++;
    printf("Required packets: %d\n", total);

    int wsize = 5;
    int wstart = 0;

    int cur = 0;

    fd_set readset;
    struct timeval timeout = {1, 0};   // 1 sec timeout



    while (1) {

      // Received an ack
      if(recvfrom(sock,&incoming, sizeof(incoming),0,(struct sockaddr *)&client,&clientlen) > 0) {
        printf(" received something\n");
      }

      // Check for timeouts here
      // Send packets here
      else {
        printf(" listening\n");
        sleep(5);
      }
    }


    while (cur < total)
    {
      int wend = wstart + wsize;
      while (cur < wend)
      {
        bzero((char *) &outgoing, sizeof(outgoing));
        outgoing.type = 1; // data packet
        outgoing.seq = wstart + cur;
        fseek(fp, outgoing.seq * PACKET_DATA_SIZE, SEEK_SET);
        outgoing.size = fread(outgoing.data, 1, PACKET_DATA_SIZE, fp);

        // send next packet in window
        if (sendto(sock, &outgoing, sizeof(outgoing), 0, (struct sockaddr*) &client, clientlen) < 0)
        	error("ERROR sending packet\n");

        print_packet(outgoing, 1);
        cur++;

        // HOW TO ATTACH A TIMER ON EACH PACKET?????



      }
    }

    // Divide file into packets

    // Send each packet as they are ready

    //write(1, buf + HEADER_SIZE, PACKET_SIZE - HEADER_SIZE);
    // printf ("DATA: %s", buf + HEADER_SIZE);
    //write(1,buf+HEADER_SIZE,n);


    // n = sendto(sock,"Got your message\n",17, 0,(struct sockaddr *)&client,clientlen);
    // if (n  < 0) error("sendto");

  }

  return 0;
}
