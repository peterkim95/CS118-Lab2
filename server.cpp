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

void print_usage(char* filename) {
    printf("\n");
    printf("usage: %s -p <port_number> [-w <window_size>] [-t <timeout>]\n", filename);
    printf("                [-l <loss_probability>] [-c <corruption_probability>]\n");
    printf("\n");
    printf("Options:\n");
    printf("     -p <port_number>\n");
    printf("         Set the port number that the server should be listening to\n");
    printf("     -w <window_size>\n");
    printf("         Set the window size in bytes.  Default is 5000\n");
    printf("     -t <timeout>\n");
    printf("         Set the timeout in milliseconds.  Default is 10000\n");
    printf("     -l <loss_probability>\n");
    printf("         Set the probability that a packet sent gets lost.\n");
    printf("         Default is 0.  <loss_prabability> should be between 0 and 1\n");
    printf("     -c <corruption_probability>\n");
    printf("         Set the probability that a packet sent gets corrupted.\n");
    printf("         Default is 0.  <corruption_probability> should be between 0 and 1\n");
    printf("\n");

    return;
}


int get_next_seq_num(int current_seq_num, int window_size) {
  int next_seq_num = current_seq_num + PACKET_SIZE;

  if (next_seq_num <= MAX_SEQ_NUM) {
    return next_seq_num;
  }
  else {
    return 0;
  }
}

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

   outgoing.seq = current_seq_num;
   current_seq_num = get_next_seq_num(current_seq_num, window_size);

   // Move the pointer pointing to where we finished reading from
   window_end += outgoing.size;

   // Check if this is the final packet
   if (feof(fp)) {
     outgoing.type = FIN;
     fclose(fp);
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
   window_slot.seq_num = outgoing.seq;
   window_slot.got_ack = false;
   window.push_back(window_slot);

   // Send the packet
   if (sendto(sock, &outgoing, sizeof(outgoing), 0, (struct sockaddr*) &client, clientlen) < 0)
      error("ERROR sending packet\n");

   // print_packet(outgoing, 1);
   // printout header stuff
   printf("Sent packet: ");
   printf(" [Type: %d]",         outgoing.type);
   printf(" [Seq #: %5d]",        outgoing.seq);
   printf(" [Payload size: %4d]", outgoing.size);
   printf("\n");

   return;
}






int main(int argc, char **argv)
{
  int sock, length, n;
  int seq_num;
  size_t window_size = 5; // TODO: input
  int current_seq_num = -1;
  long timeout = 10000;    // TODO
  int window_end = 0;         // holds byte offset of where to read next in the file
  socklen_t clientlen;
  struct sockaddr_in server, client;

  Packet incoming, outgoing;
  list<bpacket> timer_queue;
  list<Window_slot> window;
  char* port_no;
  float lost_probability = 0;
  float corruption_probability = 0;


  // Handle commandline arguments
  extern char *optarg;
  int getopt_val;
  bool pflag=false;
  while ((getopt_val = getopt(argc, argv, "p:l:c:w:t:")) != -1) {
    switch(getopt_val) {
      case 'p':
        pflag = 1;
        port_no = optarg;
        break;
      case 'l':
        lost_probability = strtof(optarg, NULL);
        if (lost_probability > 1) {
          printf("Probabilty of a lost packet must between 0 and 1\n");
          exit(1);
        }
        break;
      case 'c':
        corruption_probability = strtof(optarg, NULL);
        if (corruption_probability > 1) {
          printf("Probabilty of a corrupted packet must between 0 and 1\n");
          exit(1);
        }
        break;
      case 'w':
        window_size = atoi(optarg);
        break;
      case 't':
        timeout = atoi(optarg);
        break;
      case '?':
        print_usage(argv[0]);
        exit(1);
        break;
    }
  }
  if (pflag == false) {
    printf("\n  ** You must specify a port ** \n");
    print_usage(argv[0]);
    exit(1);
  }
  printf("Starting the server\n");
  printf("  - Port:  %s\n", port_no);
  printf("  - Window size: %ld bytes\n", window_size);
  printf("  - Timeout: %lu milliseconds\n", timeout);
  printf("  - Probabilty of a lost packet: %.2f\n", lost_probability);
  printf("  - Probabilty of a corrupted packet: %.2f\n", corruption_probability);



  // Create UDP socket
  sock=socket(AF_INET, SOCK_DGRAM, 0);
  if (sock < 0) error("Opening socket");
  length = sizeof(server);
  bzero(&server,length);
  server.sin_family=AF_INET;
  server.sin_addr.s_addr=INADDR_ANY;
  server.sin_port=htons(atoi(port_no));
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
    window_end = 0;
    current_seq_num = 0;
    while((window.size() < window_size ) && (window_end < fsize)) {
       send_packet(outgoing, window_end, timer_queue, sock, fp, client, clientlen, window, current_seq_num, window_size);
    }


    // Continously listen for acks, and send more packets when window opens.
    while (1) {

      // Received an ack
      if(recvfrom(sock,&incoming, sizeof(incoming),0,(struct sockaddr *)&client,&clientlen) > 0) {

        // Check ack's sequence number to see what packet got received
        seq_num = get_seq_num(incoming);

        printf("Received an ack:  Seq #%7d\n", seq_num);

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
          printf(" ====== Completed file transfer ======\n\n");
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

        // If there is a timeout, we need to resend the packet and restart the timer
        if (time_delta > timeout) {

          // Move from the front of the timer queue to the end, with a new timestamp
          list<bpacket>::iterator it = timer_queue.begin();
          bpacket retransmitted_packet;
          retransmitted_packet.p = it->p;
          retransmitted_packet.t = get_current_timestamp();
          timer_queue.erase(it);
          timer_queue.push_back(retransmitted_packet);

          // Resend
          printf(" * timeout: retransmitted seq# %d\n", retransmitted_packet.p.seq);
          if (sendto(sock, &retransmitted_packet.p, sizeof(retransmitted_packet.p), 0, (struct sockaddr*) &client, clientlen) < 0) {
             error("ERROR sending packet\n");
          }
        }

        else {
          //printf(" listening\n");
          //sleep(5);
        }
      }
    }
  }

  return 0;
}
