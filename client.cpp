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
#include <time.h>

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

  bool is_complete = false;
  bool total[10000];

  // set by user later
  int ploss = 0.1;
  int pcorr = 0.2;

  srand (time(NULL));

  FILE* fp = fopen("receive", "w");

  char* all_data = new char [100000000];  // TODO: dynamically allocate more if needed
  char* data;

  int next_seq = 0;

  while (1)
  {
    n = recvfrom(sock, &incoming, sizeof(incoming), 0, (struct sockaddr *)&from, &length);
    if (n < 0) error("recvfrom");
    print_packet(incoming, 0);

    // 0. Emulate corruption and loss
    double r = ((double)rand() / (double)RAND_MAX);
    if (r < ploss)
    {
      printf("This packet is lost. Discarding...\n");
      continue;
    }

    r = ((double)rand() / (double)RAND_MAX);
    if (r < pcorr)
    {
      printf("This packet is corrupted. Discarding...\n");
      continue;
    }

    total[incoming.seq] = true;
    // 1. Process Packet
    if (next_seq != incoming.seq) // received expected packet according to sequence
    {
      int i = incoming.seq;
      while (total[i] == true){
        i++;
      }
      next_seq = i;
    }
    else  // received out-of-order packet
    {
      printf("out-of-order packet received!\n");

    }

    int offset;
    offset = incoming.seq * incoming.size;   // size should be 1008 for normal data packet; otherwise whatever's left for the final one

    // Write data
    memcpy(all_data + offset, incoming.data, incoming.size);


    // 2. Send ACK
    bzero((char *) &outgoing, sizeof(outgoing));
    outgoing.type = 2;
    outgoing.seq = incoming.seq;
    outgoing.size = 0;  // empty data, just an ack
    // strcpy(outgoing.data, filename);

    if (sendto(sock, &outgoing, sizeof(outgoing), 0, (struct sockaddr*) &server, length) < 0) {
      error("ERROR - Failed to write to socket in sending ack");
    }


    if (incoming.type == 3)  // final data packet
    {
      is_complete = true;
    }

    // File transfer complete! Break out of loop.
    if (is_complete)
      break;
  }

  //write to file
  fwrite(all_data, 1, sizeof(all_data), fp);
	free(all_data);
	fclose(fp);


  close(sock);
  return 0;
}
