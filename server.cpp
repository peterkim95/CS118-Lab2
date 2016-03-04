// sender - server
/* Creates a datagram server.  The port
   number is passed as an argument.  This
   server runs forever */

#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>

#include <fcntl.h>
#include <stdlib.h>
#include <list>
#include <iostream>

#include "packet.c"

using namespace std;


int set_seq_num(int& current_seq_num, int window_size) {
  current_seq_num += 1;
  current_seq_num = current_seq_num % window_size;
  return current_seq_num;
}

long long get_current_timestamp() {
  struct timeval te; 
  gettimeofday(&te, NULL); // get current time
  long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000; // caculate milliseconds
  return milliseconds;
}


typedef struct window_slot_struct
{
  int seq_num;  // Packet sequence number
  bool got_ack; // Whether or not the server got an ack back from the receiver for this packet
} Window_slot;


int get_seq_num(Packet& incoming) {
  return incoming.seq;
}


void send_packet(
        Packet& outgoing,
        int& window_end,
        list<bpacket>& timer_queue,
        int sock,
        FILE* fp,
        sockaddr_in client,
        socklen_t clientlen,
        list<Window_slot>& window,
        int& current_seq_num,
        int window_size
    ) {

   // Clear memory of packet with all zeros
   bzero((char *) &outgoing, sizeof(outgoing));

   // Read the next data block from the file
   fseek(fp, window_end, SEEK_SET);
   outgoing.size = fread(outgoing.data, 1, PACKET_DATA_SIZE, fp);

   outgoing.seq = set_seq_num(current_seq_num, window_size);

   // Move the pointer pointing to where we finished reading from
   window_end += outgoing.size;

   // Check if this is the final packet
   if (feof(fp)) {
     outgoing.type = FIN;
   } else {
     outgoing.type = DATA;
   }


   // Add packet and timestamp to timer_queue
   bpacket packet;
   packet.p = outgoing;
   packet.t = get_current_timestamp();
   timer_queue.push_back(packet);


   // If this is the first time we are sending this packet (i.e. not retransmitting it),
   // this packet is at the end of the window.
   Window_slot window_slot;
   // TODO window_slot.seq_num = outgoing.seq;
   window_slot.got_ack = false;
   window.push_back(window_slot);

   // Send the packet
   if (sendto(sock, &outgoing, sizeof(outgoing), 0, (struct sockaddr*) &client, clientlen) < 0)
      error("ERROR sending packet\n");

   // print_packet(outgoing, 1);
   // printout header stuff
   printf("Sent - Type: %d, Seq #: %d, Size: %d, Data: \n\n", outgoing.type, outgoing.seq, outgoing.size);

   return;
}






int main(int argc, char *argv[])
{
  int sock, length, n;
  int seq_num;
  size_t window_size = 5; // TODO: input
  int current_seq_num = -1;
  long timeout = 7000;    // TODO
  int window_end;         // holds byte offset of where to read next in the file
  socklen_t clientlen;
  struct sockaddr_in server, client;

  Packet incoming, outgoing;
  list<bpacket> timer_queue;
  list<Window_slot> window;


  // Handle commandline arguments
  if (argc < 2)
  {
    fprintf(stderr, "ERROR, no port provided\n");
    exit(0);
  }


  // Create UDP socket
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

  int flags = fcntl(sock, F_GETFL, 0);         // get the flags currently set for the socket
  fcntl( sock, F_SETFL, flags | O_NONBLOCK );  // make the socket nonblocking


  // Continously run server, listening for requests
  while (1)
  {

    // Listen until we get a request for a file
    if ((n = recvfrom(sock,&incoming, sizeof(incoming),0,(struct sockaddr *)&client,&clientlen)) < 0) {
      sleep(1);
      continue;
    }


    // Got a request for a file
    char* filename;
    filename = incoming.data;
    printf("Got a request for file:%s\n", filename);

    // Check if file exists
    FILE *fp = fopen(filename, "r");
    if (!fp) error("ERROR - Requested file doesn't exist in the working directory!");

    // Figure out file size
    fseek(fp, 0L, SEEK_END);
    int fsize = (int) ftell(fp);
    fseek(fp, 0L, SEEK_SET);     // Seek back to start of file to prepare for reading
    printf("File size = %d\n", fsize);


    int total;
    total = fsize / 1024;
    if (fsize % 1024 > 0)
      total++;
    printf("Required packets: %d\n", total);



    // Send the initial packets
    // Send as many as possible until the window is full or the whole file has been sent
    while((window.size() < window_size ) && (window_end < fsize)) {
       send_packet(outgoing, window_end, timer_queue, sock, fp, client, clientlen, window, current_seq_num, window_size);
    }


    // Continously listen for acks, and send more packets when window opens.
    while (1) {

      // Received an ack
      if(recvfrom(sock,&incoming, sizeof(incoming),0,(struct sockaddr *)&client,&clientlen) > 0) {

        // Check ack's sequence number to see what packet got received
        seq_num = get_seq_num(incoming);

        // Remove the packet from timer_queue
        for (list<bpacket>::iterator it = timer_queue.begin(); it != timer_queue.end(); it++) {
          if (it->p.seq == seq_num) {
            timer_queue.erase(it);
            break;
          }
        }

        // Mark that packet as ack'ed in the window
        for (list<Window_slot>::iterator it = window.begin(); it != window.end(); it++) {
          if (it->seq_num == seq_num) {
            it->got_ack = true;
            break;
          }
        }

        // Slide window over as much as possible
        list<Window_slot>::iterator it = window.begin();
        while ((it != window.end()) && (it->got_ack)) {
           it = window.erase(it);
        }

        // Send next packet
        // Add timer to queue
        // Add to end of window
        while((window.size() < window_size ) && (window_end < fsize)) {
           send_packet(outgoing, window_end, timer_queue, sock, fp, client, clientlen, window, current_seq_num, window_size);
        }

        // If there are no more packets to send, break out of this loop (stop listening for acks)
        // so the server can listen for the next file request.
        if (timer_queue.empty() && window_end >= fsize) {
          break;
        }
      }

      // Check for timeouts here
      // Send packets here
      else {

        // Get current time
        long long current_timestamp = get_current_timestamp();

        // Compare current time with oldest timestamp on the queue to get time_delta
        list<bpacket>::iterator it = timer_queue.begin();
        long long oldest_timestamp = it->t;
        long long time_delta = current_timestamp - oldest_timestamp;

        //   dequeue
        //   resend packet
        //   add packet to end of queue
        if (time_delta > timeout) {

          // Move from the front of the timer queue to the end, with a new timestamp
          list<bpacket>::iterator it = timer_queue.begin();
          bpacket retransmitted_packet;
          retransmitted_packet.p = it->p;
          retransmitted_packet.t = get_current_timestamp();
          timer_queue.erase(it);
          timer_queue.push_back(retransmitted_packet);

          // Resend
          printf(" * timeout: retransmitted %d\n", retransmitted_packet.p.seq);
          if (sendto(sock, &outgoing, sizeof(retransmitted_packet.p), 0, (struct sockaddr*) &client, clientlen) < 0) {
             error("ERROR sending packet\n");
          }

          //for (list<bpacket>::iterator it = timer_queue.begin(); it != timer_queue.end(); it++) { }
        }

        else {
          printf(" listening\n");
          sleep(5);
        }
      }
    }
  }

  return 0;
}
