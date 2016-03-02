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
#include <vector>

#include "packet.c"

using namespace std;

int main(int argc, char *argv[])
{
  int sock, n;
  unsigned int length;
  Packet incoming, outgoing;
  struct sockaddr_in server, from;
  struct hostent *hp;
  char buffer[1024];
  char* filename;

  if (argc != 4)
  {
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

  bzero((char *) &outgoing, sizeof(outgoing));
  outgoing.type = 0;
  outgoing.seq = 0;
  outgoing.size = strlen(filename) + 1;
  strcpy(outgoing.data, filename);

  // Send initial file request
  if (sendto(sock, &outgoing, sizeof(outgoing), 0, (const struct sockaddr*)&server, length) < 0)
    error("Sendto");
  print_packet(outgoing, 1);

  n = recvfrom(sock, &incoming, sizeof(incoming), 0, (struct sockaddr *)&from, &length);
  if (n < 0) error("recvfrom");
  // write(1,"Got an ack: ",12);
  //
  // write(1,buffer,n);
  print_packet(incoming, 0);

  close(sock);
  return 0;
}
