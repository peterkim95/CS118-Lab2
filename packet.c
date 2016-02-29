#define PACKET_DATA_SIZE 1008 // 1024 - 4(INT) = 1024 - (4*4) = 1008

struct packet {
  int type;	// 0: Request, 1: Data, 2: ACK, 3: FIN
  int seq;	// Packet sequence number
  int size;	// Data size
  int checksum;   // To detect corruption

  char data[PACKET_DATA_SIZE];
};

void print_packet(struct packet p, int x) {
	if (x == 0)
		printf("Received - Type: %d, Seq #: %d, Size: %d, Data: \n%s\n", p.type, p.seq, p.size, p.data);
	else
		printf("Sent - Type: %d, Seq #: %d, Size: %d, Data: \n%s\n", p.type, p.seq, p.size, p.data);
	return;
}

void error(const char *msg)
{
  perror(msg);
  exit(0);
}
