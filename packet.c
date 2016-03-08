//typedef enum { false, true } bool;
#include <tr1/functional>
#define PACKET_DATA_SIZE 984 // 1000 - 4(INT) = 1000 - (4*4) = 984

const int MAX_SEQ_NUM = 30000;  // From the spec: 30 Kbytes
const int PACKET_SIZE = 1000;

enum Packet_type { DATA, ACK, FIN };

std::tr1::hash<char*> my_hash;

typedef struct packet
{
  int type;	// 0: Request, 1: Data, 2: ACK, 3: FIN
  int seq;	// Packet sequence number
  int size;	// Data size
  size_t checksum;   // To detect corruption

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
		printf("Received - Type: %d, Seq #: %d, Size: %d\n", p.type, p.seq, p.size);
        //printf("%s\n", p.data);
	else
		printf("Sent - Type: %d, Seq #: %d, Size: %d, Data: \n%s\n", p.type, p.seq, p.size, p.data);
	return;
}

void error(const char *msg)
{
  perror(msg);
  exit(0);
}

void corrupt_packet(Packet* p)
{

  const char* c;
  c = "corrupted...";
  strcpy(p->data, c);
  return;
}
