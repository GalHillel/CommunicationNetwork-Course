#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>


#define P 80            // port number P
#define P_PLUS_1 81     // port number P+1
#define MAX_BUF_LEN 100 // maximum buffer length

int main(int argc, char *argv[]) {
  // check for correct number of command line arguments
  if (argc != 2) {
    fprintf(stderr, "Usage: %s hostname\n", argv[0]);
    exit(1);
  }

  // create a socket for sending datagrams
  int sock_out = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock_out < 0) {
    perror("Error creating outgoing socket");
    exit(1);
  }

  // create a socket for receiving datagrams
  int sock_in = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock_in < 0) {
    perror("Error creating incoming socket");
    exit(1);
  }

  // set up server address for outgoing socket
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(P_PLUS_1);
  inet_aton(argv[1], &server_addr.sin_addr);
  memset(server_addr.sin_zero, '\0', sizeof server_addr.sin_zero);

  // bind incoming socket to port P
  struct sockaddr_in my_addr;
  my_addr.sin_family = AF_INET;
  my_addr.sin_port = htons(P);
  my_addr.sin_addr.s_addr = INADDR_ANY;
  memset(my_addr.sin_zero, '\0', sizeof my_addr.sin_zero);
  if (bind(sock_in, (struct sockaddr *)&my_addr, sizeof my_addr) < 0) {
    perror("Error binding incoming socket to port P");
    exit(1);
  }

  // enter infinite loop
  while (1) {
    // receive datagram from port P
    char buf[MAX_BUF_LEN];
    struct sockaddr_in sender_addr;
    socklen_t sender_len = sizeof sender_addr;
    int bytes_received = recvfrom(sock_in, buf, MAX_BUF_LEN, 0,
                                  (struct sockaddr *)&sender_addr, &sender_len);
    if (bytes_received < 0) {
      // add back carry outs from top 16 bits to low 16 bitsperror("Error
      // receiving datagram on port P");
      exit(1);
    }

    // simulate unreliable network by discarding datagram with 50% probability
    float rand_num = ((float)random()) / ((float)RAND_MAX);
    if (rand_num > 0.5) {
      // forward datagram to host on port P+1
      int bytes_sent =
          sendto(sock_out, buf, bytes_received, 0,
                 (struct sockaddr *)&server_addr, sizeof server_addr);
      if (bytes_sent < 0) {
        perror("Error forwarding datagram to host on port P+1");
        exit(1);
      }
    }
  }
  // close sockets
  close(sock_out);
  close(sock_in);

  return 0;
}