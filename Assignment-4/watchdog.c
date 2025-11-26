#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>


#define WATCHDOG_PORT 3000
#define MAX_CLIENT_ADDR_LEN 16
#define PING_STATUS_LEN 5
#define TRUE 1

int main() {
  int watchdog_sock = socket(AF_INET, SOCK_STREAM, 0);
  if (watchdog_sock == -1) {
    printf("Socket not created: %d", errno);
    exit(0);
  }

  int enable = 1, status;
  status =
      setsockopt(watchdog_sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
  if (status == -1) {
    printf("setsockopt() failed with error code : %d", errno);
    close(watchdog_sock);
    exit(0);
  }

  struct sockaddr_in server_address;
  memset(&server_address, 0, sizeof(server_address));
  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = INADDR_ANY;
  server_address.sin_port = htons(WATCHDOG_PORT);

  int bind_status = bind(watchdog_sock, (struct sockaddr *)&server_address,
                         sizeof(server_address));
  if (bind_status == -1) {
    printf("Bind failed with error code : %d", errno);
    close(watchdog_sock);
    exit(0);
  }

  int listen_status = (int)listen(watchdog_sock, 5);
  if (listen_status == -1) {
    printf("Listen failed with error code : %d", errno);
    close(watchdog_sock);
    exit(0);
  }

  struct sockaddr_in client_address;
  socklen_t client_address_len = sizeof(client_address);
  memset(&client_address, 0, client_address_len);

  int client_socket = accept(watchdog_sock, (struct sockaddr *)&client_address,
                             &client_address_len);
  if (client_socket == -1) {
    printf("listen_status failed with error code : %d", errno);
    close(client_socket);
    exit(0);
  }

  int flag1 = 1;
  send(client_socket, &flag1, 1, 0);

  char dest_ip[MAX_CLIENT_ADDR_LEN];
  bzero(dest_ip, MAX_CLIENT_ADDR_LEN);
  int receive = (int)recv(client_socket, dest_ip, MAX_CLIENT_ADDR_LEN, 0);
  if (receive < 1) {
    printf("Destination IP wasn't received correctly");
    close(client_socket);
    return -1;
  }

  char ping_status[PING_STATUS_LEN];
  int timer = 0;

  while (TRUE) {
    recv(client_socket, ping_status, PING_STATUS_LEN, 0);
    if (strcmp("ping", ping_status) == 0) {
      // printf("Started timer\n");
      fcntl(client_socket, F_SETFL, O_NONBLOCK);
      while (timer < 10) {
        timer++;
        recv(client_socket, ping_status, PING_STATUS_LEN, 0);
        if (strcmp("pong", ping_status) == 0) {
          timer = 0;
          fcntl(client_socket, F_SETFL, 0);
          break;
        }
        sleep(1);
      }
      if (timer == 10) {
        printf("Disconnected\n");
        close(client_socket);
        break;
      }
    }
  }

  return 0;
}
