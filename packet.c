typedef enum { false, true } bool;

#define PACKET_DATA_SIZE 1008 // 1024 - 4(INT) = 1024 - (4*4) = 1008

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
  time_t t;
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
  char* c;
  c = "corrupted";
  strcpy(p.data, c);
  return;
}

// Generic Queue Implementation in C
// Taken from http://www.sanfoundry.com/c-program-queue-using-linked-list/

// struct node
// {
//   BPacket info;
//   struct node *ptr;
// } *front, *rear, *temp, *front1;
//
// BPacket frontelement();
// void enq(BPacket data);
// void deq();
// int empty();
// void display();
// void create();
// void queuesize();
//
// int count = 0;
//
// /* Create an empty queue */
// void create()
// {
//   front = rear = NULL;
// }
//
// /* Enqueing the queue */
// void enq(BPacket data)
// {
//     if (rear == NULL)
//     {
//         rear = (struct node *)malloc(1*sizeof(struct node));
//         rear->ptr = NULL;
//         rear->info = data;
//         front = rear;
//     }
//     else
//     {
//         temp=(struct node *)malloc(1*sizeof(struct node));
//         rear->ptr = temp;
//         temp->info = data;
//         temp->ptr = NULL;
//
//         rear = temp;
//     }
//     count++;
// }
//
// /* Dequeing the queue */
// void deq()
// {
//     front1 = front;
//
//     if (front1 == NULL)
//     {
//         printf("\n Error: Trying to display elements from empty queue");
//         return;
//     }
//     else
//         if (front1->ptr != NULL)
//         {
//             front1 = front1->ptr;
//             // printf("\n Dequed value : %d", front->info);
//             free(front);
//             front = front1;
//         }
//         else
//         {
//             // printf("\n Dequed value : %d", front->info);
//             free(front);
//             front = NULL;
//             rear = NULL;
//         }
//         count--;
// }
//
// /* Returns the front element of queue */
// BPacket frontelement()
// {
//   if ((front != NULL) && (rear != NULL))
//     return(front->info);
//   // else
//     // return 0;
// }
//
// /* Display if queue is empty or not */
// int empty()
// {
//   if ((front == NULL) && (rear == NULL))
//     return 1;
//   else
//     return 0;
// }
