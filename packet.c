//typedef enum { false, true } bool;

#define PACKET_DATA_SIZE 1008 // 1024 - 4(INT) = 1024 - (4*4) = 1008

const int MAX_SEQ_NUM = 30000;  // From the spec: 30 Kbytes
const int PACKET_SIZE = 1024;

enum Packet_type { DATA, ACK, FIN };

typedef struct packet
{
  int type;	// 0: Request, 1: Data, 2: ACK, 3: FIN
  int seq;	// Packet sequence number
  int size;	// Data size
  int checksum;   // To detect corruption

  char data[PACKET_DATA_SIZE];
} Packet;

typedef struct bpacket
{
  long long t;
  Packet p;
} BPacket;

void print_packet(Packet p, int x)
{
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

int hash(char* data)
{
  return 1;
}

void corrupt_packet(Packet p)
{
  //char* c;
  //c = "corrupted";
  //strcpy(p.data, c);
  return;
}
