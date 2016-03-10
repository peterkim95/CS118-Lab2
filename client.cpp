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
#include <list>
#include <iostream>
#include <string>

#include "packet.c"

using namespace std;

bool interactive_mode = false;

typedef struct window_slot_struct
{
  int seq_num;  // Packet sequence number
  Packet packet;
  bool received;
} Window_slot;


int get_next_seq_num(int current_seq_num, int window_size) {
  int next_seq_num = current_seq_num + PACKET_SIZE;

  if (next_seq_num <= MAX_SEQ_NUM) {
    return next_seq_num;
  }
  else {
    return 0;
  }
}

int main(int argc, char *argv[])
{
  int sock, n;
  unsigned int length;
  Packet incoming, outgoing;
  struct sockaddr_in server, from;
  struct hostent *hp;
  char* filename;
  list<Window_slot> window;
  int window_size;

  if (argc != 7)
  {
    printf("Usage: ./client host port filename pl pc window_size\n");
    exit(1);
  }

  // Determine window size
  window_size = atoi(argv[6]) / 1000;

  // Get requested filename
  filename = argv[3];

  // Get ploss and pcorrupt
  double ploss;
  double pcorr;
  ploss = atof(argv[4]);
  pcorr = atof(argv[5]);

  if (ploss < 0 || ploss > 1 || pcorr < 0 || pcorr > 1)
  {
    printf("Your probabilities of loss and corruption must be 0 <= p <= 1!");
    exit(1);
  }

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


  srand (time(NULL));

  FILE* fp = fopen("receive", "w");


  int next_seq = 0;

  // Initialize window
  for (int i = 0; i < window_size; i++) {
    Window_slot window_slot;
    window_slot.received = false;
    window_slot.seq_num = next_seq;
    window.push_back(window_slot);

    next_seq = get_next_seq_num(next_seq, window_size);
  }


  // Listen for packets
  while (1)
  {
    n = recvfrom(sock, &incoming, sizeof(incoming), 0, (struct sockaddr *)&from, &length);
    if (n < 0) error("recvfrom");
    print_packet(incoming, 0);

    // 0. Emulate corruption and loss
    double r = ((double)rand() / (double)RAND_MAX);
    //printf("(%f)\n", r);
    //printf("[%d]\n", ploss);
    if (r < ploss)
    {
      printf("This packet is lost. Discarding...\n");
      continue;
    }

    r = ((double)rand() / (double)RAND_MAX);
    //printf("(%f)\n", r);
    //printf("[%f]\n", pcorr);
    if (r < pcorr) {
       //printf("CORRUPTN\n");
       //corrupt_packet(&incoming);
       printf("This packet is corrupted. Discarding...\n");
       continue;
    }
    //std::tr1::hash<char*> my_hash;
    //printf("%u \n", incoming.checksum);
    //printf("%u \n", my_hash(incoming.data));
    //if (my_hash(incoming.data) != incoming.checksum)
    //{
      //printf("This packet is corrupted. Discarding...\n");
      //continue;
    //}

    if (interactive_mode) {
      string input;
      printf("Make this packet lost?\n");
      cin >> input;
      if (input[0] == 'y') {
        continue;
      }
    }

    // 1. Process Packet
    // Mark the packt with the proper sequence number as received in our window
    for (list<Window_slot>::iterator it = window.begin(); it != window.end(); it++) {
      if (it->seq_num == incoming.seq) {
        it->received = true;
        it->packet = incoming;
        break;
      }
    }

    // Slide window over as much as possible and write data
    list<Window_slot>::iterator it = window.begin();
    while(it->received == true) {
        // Write data
        fwrite(it->packet.data, 1, it->packet.size, fp);

        // If that was the last packet written we've written the whole file
        if (it->packet.type == FIN) {
          is_complete = true;
        }

        // Remove from front of queue
        it = window.erase(it);

        // Slide window over by adding a slot to the end queue to make up for deleting from the front of the queue
        Window_slot window_slot;
        window_slot.received = false;
        window_slot.seq_num = next_seq;
        window.push_back(window_slot);
        next_seq = get_next_seq_num(next_seq, window_size);
    }

    // 2. Send ACK
    bzero((char *) &outgoing, sizeof(outgoing));
    outgoing.type = ACK;
    outgoing.seq = incoming.seq;
    outgoing.size = 0;  // empty data, just an ack
    if (sendto(sock, &outgoing, sizeof(outgoing), 0, (struct sockaddr*) &server, length) < 0) {
      error("ERROR - Failed to write to socket in sending ack");
    }
    print_packet(outgoing, 1);
    // File transfer complete! Break out of loop.
    if (is_complete) {
      break;
    }
  }

  // Close the file pointer
  fclose(fp);

  // Close the socket
  close(sock);
  return 0;
}
