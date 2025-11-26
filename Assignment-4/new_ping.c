#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <resolv.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>


// run 2 programs using fork + exec
// command: make clean && make all && ./partb

#define ICMP_HDRLEN 8
#define WATCHDOG_IP "127.0.0.1"
#define WATCHDOG_PORT 3000
#define TRUE 1

/**
 * Calculates the checksum for the given data.
 * @param p_address Pointer to the data.
 * @param len Length of the data.
 * @return The calculated checksum.
 */
unsigned short calculate_checksum(unsigned short *p_address, int len) {
  int sum = 0;
  unsigned short *w = p_address;
  unsigned short answer = 0;

  for (; len > 1; len -= 2) {
    sum += *w++;
  }
  if (len == 1) {
    *((unsigned char *)&answer) = *((unsigned char *)w);
    sum += answer;
  }
  sum = (sum >> 16) + (sum & 0xffff);
  sum += (sum >> 16);
  answer = ~sum;

  return answer;
}

int main(int argc, char *strings[]) {
  if (argc != 2) {
    printf("usage: %s <addr>\n", strings[0]);
    exit(0);
  }

  int tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (tcp_socket == -1) {
    printf("Socket not created: %d\n", errno);
  }

  struct sockaddr_in watchdog_addr;
  memset(&watchdog_addr, 0, sizeof(watchdog_addr));
  watchdog_addr.sin_family = AF_INET;
  watchdog_addr.sin_port = htons(WATCHDOG_PORT);
  int ip_addr =
      inet_pton(AF_INET, (const char *)WATCHDOG_IP, &watchdog_addr.sin_addr);
  if (ip_addr < 1)
    printf("inet_pton() failed %d: ", errno);

  struct hostent *hname;
  hname = gethostbyname(strings[1]);

  struct sockaddr_in dest_addr;
  bzero(&dest_addr, sizeof(dest_addr));
  dest_addr.sin_family = AF_INET;
  dest_addr.sin_port = 0;
  dest_addr.sin_addr.s_addr = *(long *)hname->h_addr;

  int raw_socket = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
  if (raw_socket < 0) {
    perror("socket");
    return -1;
  }

  int ttl = 255;
  int sockopt = setsockopt(raw_socket, SOL_IP, IP_TTL, &ttl, sizeof(ttl));
  if (sockopt != 0) {
    perror("setsockopt");
    return -1;
  }

  char *args[2];
  args[0] = "./watchdog";
  args[1] = NULL;
  int pid = fork();

  if (pid == 0) {
    // printf("In child\n");
    execvp(args[0], args);
  } else {
    sleep(2);
    int connection_status = connect(
        tcp_socket, (struct sockaddr *)&watchdog_addr, sizeof(watchdog_addr));
    if (connection_status == -1) {
      printf("Socket not connected: %d", errno);
    }

    int flag = 0;
    while (!flag) {
      recv(tcp_socket, &flag, 1, 0);
    }

    send(tcp_socket, strings[1], sizeof(strings[1]), 0);
    char data[IP_MAXPACKET] = "Ping.\n";
    int data_length = (int)strlen(data) + 1;
    char packet[IP_MAXPACKET];
    int seq = 0;
    struct timeval start, end;
    char ping_status[5];
    // to check watchdog
    int counter = 0;

    while (TRUE) {
      counter++;
      struct icmp icmphdr;
      icmphdr.icmp_type = ICMP_ECHO;
      icmphdr.icmp_code = 0;
      icmphdr.icmp_id = 18;
      icmphdr.icmp_seq = seq++;
      icmphdr.icmp_cksum = 0;

      bzero(packet, IP_MAXPACKET);
      memcpy((packet), &icmphdr, ICMP_HDRLEN);
      memcpy(packet + ICMP_HDRLEN, data, data_length);
      icmphdr.icmp_cksum = calculate_checksum((unsigned short *)(packet),
                                              ICMP_HDRLEN + data_length);
      memcpy((packet), &icmphdr, ICMP_HDRLEN);

      gettimeofday(&start, 0);
      sendto(raw_socket, packet, ICMP_HDRLEN + data_length, 0,
             (struct sockaddr *)&dest_addr, sizeof(dest_addr));

      strcpy(ping_status, "ping");
      send(tcp_socket, ping_status, sizeof(ping_status), 0);

      // to check watchdog
      if (counter > 5)
        sleep(10);
      //
      bzero(packet, IP_MAXPACKET);
      int reply_pid = fork();

      if (reply_pid == 0) {
        recv(tcp_socket, &packet, sizeof(packet), 0);
        if (strcmp("Timeout", packet) == 0) {
          printf("Received timeout.\n");
          int my_pid = getppid();
          kill(my_pid, SIGTERM);
          exit(0);
        }
      } else {
        socklen_t length = sizeof(dest_addr);
        int bytes_received =
            (int)recvfrom(raw_socket, packet, sizeof(packet), 0,
                          (struct sockaddr *)&dest_addr, &length);
        if (bytes_received > 0) {
          gettimeofday(&end, 0);

          strcpy(ping_status, "pong");
          send(tcp_socket, ping_status, sizeof(ping_status), 0);

          float time = (float)(end.tv_sec - start.tv_sec) * 1000.0f +
                       (float)(end.tv_usec - start.tv_usec) / 1000.0f;
          printf("Ping returned: %d bytes from IP = %s, Seq = %d, time = %.3f "
                 "ms\n",
                 bytes_received, strings[1], seq, time);
          sleep(1);
        }
      }
    }
    close(tcp_socket);
    close(raw_socket);
  }
  return 0;
}