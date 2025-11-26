#include <errno.h>
#include <math.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>


#define SERVER_PORT 5060
#define BUFFER_SIZE 1024

// **FUNCTION HEADERS**:

/**
 * Sends a FIN message to the client.
 * @param client_sock The client socket descriptor.
 * @param buffer The buffer to use for receiving the ACK.
 * @return 0 on success, -1 on error.
 */
int send_fin(int client_sock, char *buffer);

/**
 * Sends the file size to the client.
 * @param client_sock The client socket descriptor.
 * @param buffer The buffer to use for sending.
 * @param size The size of the file.
 * @return 0 on success, -1 on error.
 */
int send_file_size(int client_sock, char *buffer, int size);

/**
 * Asks the client for a key and verifies it.
 * @param client_sock The client socket descriptor.
 * @param client_key The buffer to store the client's key.
 * @param server_key The server's key to compare against.
 * @return 0 on success, -1 on error.
 */
int get_key(int client_sock, char *client_key, char *server_key);

/**
 * Sends the file content to the client.
 * @param fp The file pointer.
 * @param client_sock The client socket descriptor.
 * @param size The total size of the file.
 * @param counter The current number of bytes sent.
 * @param buffer The buffer to use for sending.
 * @return The updated counter (bytes sent) on success, -1 on error.
 */
int send_file(FILE *fp, int client_sock, int size, int counter,
              char buffer[BUFFER_SIZE]);

/**
 * Sends an AGAIN message to the client.
 * @param client_sock The client socket descriptor.
 * @param buffer The buffer to use for receiving the ACK.
 * @return 0 on success, -1 on error.
 */
int send_again(int client_sock, char *buffer);

/**
 * Sends an END message to the client.
 * @param client_sock The client socket descriptor.
 * @param buffer The buffer to use for receiving the ACK.
 * @return 0 on success, -1 on error.
 */
int send_end(int client_sock, char *buffer);

/**
 * Gets the size of the file.
 * @param fp The file pointer.
 * @return The size of the file in bytes.
 */
int get_file_size(FILE *fp);

/**
 * Returns the minimum of two integers.
 * @param a First integer.
 * @param b Second integer.
 * @return The smaller of the two integers.
 */
int min(int a, int b);

int main() {
  signal(SIGPIPE, SIG_IGN);

  int temp = 0;
  int listen_sock = -1;
  listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  if (listen_sock == -1) {
    printf("Error : Listen socket creation failed.\n");
    return 0;
  }

  int reuse = 1;
  temp = setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int));
  if (temp < 0) {
    printf("Error : Failed to set congestion control algorithm to reno.\n");
    return 0;
  }

  struct sockaddr_in server_address;
  memset(&server_address, 0, sizeof(server_address));

  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = INADDR_ANY;
  server_address.sin_port = htons(SERVER_PORT);

  temp = bind(listen_sock, (struct sockaddr *)&server_address,
              sizeof(server_address));
  if (temp == -1) {
    printf("Error: Binding failed.\n");
    close(listen_sock);
    return -1;
  }

  temp = listen(listen_sock, 3);
  if (temp == -1) {
    printf("Error: Listening failed.");
    close(listen_sock);
    return -1;
  }

  struct sockaddr_in client_address;
  socklen_t client_addr_len = sizeof(client_address);

  while (1) {
    printf("Waiting for a connection...\n");

    memset(&client_address, 0, sizeof(client_address));
    client_addr_len = sizeof(client_address);
    int client_sock = accept(listen_sock, (struct sockaddr *)&client_address,
                             &client_addr_len);

    if (client_sock == -1) {
      printf("Error: Accepting a client failed.");
      close(listen_sock);
      return -1;
    }

    printf("Connected to client!\n");

    FILE *fp = fopen("send.txt", "r");
    if (fp == NULL) {
      printf("File open error\n");
      close(client_sock);
      close(listen_sock);
      return -1;
    }

    char buffer[BUFFER_SIZE] = {0};
    int size = get_file_size(fp);
    int counter = 0;

    char client_key[10] = {0};
    char server_key[10] = {0};
    int key = 1714 ^ 6521;
    sprintf(server_key, "%d", key);

    char message[1] = {0};

    // ######################### Sending the size of the file:
    // ###############################

    printf("Sending size of the file...\n");

    temp = send_file_size(client_sock, buffer, size);
    if (temp == -1) {
      close(client_sock);
      close(listen_sock);
      return -1;
    }

    printf("Size of the file sent successfully!\n");

    while (1) {
      // ############### Setting the congestion control algorithm to reno:
      // #####################

      if (setsockopt(listen_sock, IPPROTO_TCP, TCP_CONGESTION, "reno", 6) < 0) {
        printf("Error : Failed to set congestion control algorithm to reno.\n");
        close(client_sock);
        close(listen_sock);
        return -1;
      }

      printf("CC algorithm set to reno.\n");

      // ####################### Sending the 1st part of the file:
      // #############################

      printf("Sending first part of the file...\n");

      counter = send_file(fp, client_sock, size / 2, counter, buffer);

      if (counter == -1) {
        close(client_sock);
        close(listen_sock);
        return -1;
      }

      printf("First part of the file sent successfully!\n");

      // ################ Asking the client for the key and checking if it
      // matches: #############

      printf("Asking client for key...\n");

      temp = get_key(client_sock, client_key, server_key);

      if (temp == -1) {
        close(client_sock);
        close(listen_sock);
        return -1;
      }

      printf("Keys match!\n");

      // ################ Setting the congestion control algorithm to cubic:
      // ####################

      if (setsockopt(listen_sock, IPPROTO_TCP, TCP_CONGESTION, "cubic", 6) <
          0) {
        printf("Error : Failed to set congestion control algorithm to reno.\n");
        close(client_sock);
        close(listen_sock);
        return -1;
      }

      printf("CC algorithm set to cubic.\n");

      // ######################## Sending the 2nd part of the file:
      // #############################

      printf("Sending second part of the file...\n");

      counter = send_file(fp, client_sock, size, counter, buffer);

      if (counter != size) {
        printf("Error: File size didn't match, sending failed.\n");
        close(client_sock);
        close(listen_sock);
        return -1;
      }

      printf("Second part of the file sent successfully!\n");

      // ############## Sending the client that the server is done sending the
      // file: ##############

      printf("Letting the client know we finished sending the file...\n");

      temp = send_fin(client_sock, buffer);

      if (temp == -1) {
        close(client_sock);
        close(listen_sock);
        return -1;
      }

      printf("Client acknowledged!\n");

      // ################ Asking sender's permission to send the file again:
      // ######################

      printf("Do you want to send the file again? If so - enter 'y'. If not, "
             "enter anything else. ");

      scanf("%s", message);

      if (strcmp(message, "y") == 0) {
        counter = 0;
        fseek(fp, 0, SEEK_SET);

        temp = send_again(client_sock, buffer);

        if (temp == -1) {
          close(client_sock);
          close(listen_sock);
          return -1;
        }
      } else {
        break;
      }
    }

    printf("Asking the client to close connection...\n");

    temp = send_end(client_sock, buffer);

    if (temp == -1) {
      close(client_sock);
      close(listen_sock);
      return -1;
    }

    printf("Client closed the connection!\n");

    fclose(fp);
    close(client_sock);
  }

  close(listen_sock);
  printf("\n");

  return 0;
}

// **THE FUNCTIONS** :

int send_fin(int client_sock, char *buffer) {
  int send_result = send(client_sock, "FIN", 4, 0);

  if (send_result == -1) {
    printf("Error : Sending failed.\n");
    return -1;
  } else if (send_result == 0) {
    printf("Error : Client's socket is closed, couldn't send to it.\n");
    return -1;
  } else if (send_result != 4) {
    printf("Error : Client received a corrupted buffer.\n");
    return -1;
  }

  int recv_result = recv(client_sock, buffer, 4, 0);

  if (recv_result < 0) {
    printf("Error : Receiving failed.\n");
    return -1;
  } else if (recv_result == 0) {
    printf("Error : Client's socket is closed, nothing to receive.\n");
    return -1;
  } else if (strcmp(buffer, "ACK")) {
    printf("Error : Server received a corrupted buffer.\n");
    return -1;
  }

  bzero(buffer, 1);

  return 0;
}

int send_file_size(int client_sock, char *buffer, int size) {
  sprintf(buffer, "%d", size);

  int send_size = send(client_sock, buffer, strlen(buffer) + 1, 0);

  if (send_size == -1) {
    printf("Error : Sending dropped.\n");
    return -1;
  } else if (send_size == 0) {
    printf("Error : Client's socket is closed, couldn't send to it.\n");
    return -1;
  } else if (send_size != strlen(buffer) + 1) {
    printf("Error : Server sent a corrupted buffer.\n");
    return -1;
  }

  bzero(buffer, (int)(strlen(buffer) + 1));

  int recv_result = recv(client_sock, buffer, 4, 0);

  if (recv_result < 0) {
    printf("Error : Receiving failed.\n");
    return -1;
  } else if (recv_result == 0) {
    printf("Error : Client's socket is closed, nothing to receive.\n");
    return -1;
  } else if (strcmp(buffer, "ACK") != 0) {
    printf("Error : Server received a corrupted buffer.\n");
    return -1;
  }

  bzero(buffer, 4);
}

int get_key(int client_sock, char *client_key, char *server_key) {
  int send_result = send(client_sock, "SEND KEY", 9, 0);

  if (send_result == -1) {
    printf("Error : Sending faied.\n");
    return -1;
  } else if (send_result == 0) {
    printf("Error : Client's socket is closed, couldn't send to it.\n");
    return -1;
  } else if (send_result != 9) {
    printf("Error : Client received a corrupted buffer.\n");
    return -1;
  }

  int recv_key = recv(client_sock, client_key, 5, 0);

  if (recv_key < 0) {
    printf("Error : Receiving failed.\n");
    return -1;
  } else if (recv_key == 0) {
    printf("Error : Client's socket is closed, nothing to receive.\n");
    return -1;
  } else if (recv_key != 5) {
    printf("Error : Server received a corrupted buffer.\n");
    return -1;
  } else if ((strcmp(server_key, client_key)) != 0) {
    printf("Error : Keys don't match.\n");
    return -1;
  }

  int send_ok = send(client_sock, "OK", 3, 0);

  if (send_ok == -1) {
    printf("Error : Sending failed.\n");
    return -1;
  } else if (send_ok == 0) {
    printf("Error : Client's socket is closed, couldn't send to it.\n");
    return -1;
  } else if (send_ok != 3) {
    printf("Error : Client received a corrupted buffer.\n");
    return -1;
  }

  bzero(client_key, 10);

  return 0;
}

int send_file(FILE *fp, int client_sock, int size, int counter,
              char buffer[BUFFER_SIZE]) {
  int num_bytes = min(BUFFER_SIZE, size - counter);

  while (counter < size && fread(buffer, num_bytes, 1, fp) != 0) {
    int send_result = send(client_sock, buffer, num_bytes, 0);

    if (send_result == -1) {
      printf("Error : Sending failed.\n");
      return -1;
    } else if (send_result == 0) {
      printf("Error : Client's socket is closed, couldn't send to it.\n");
      return -1;
    } else if (send_result != num_bytes) {
      printf("Error : Client received a corrupted buffer.\n");
      return -1;
    }

    bzero(buffer, num_bytes);

    int recv_result = recv(client_sock, buffer, 4, 0);

    if (recv_result < 0) {
      printf("Error : Receiving failed.\n");
      return -1;
    } else if (recv_result == 0) {
      printf("Error : Client's socket is closed, nothing to receive.\n");
      return -1;
    } else if (strcmp(buffer, "ACK") != 0) {
      printf("Error : Server received a corrupted buffer.\n");
      return -1;
    }

    bzero(buffer, 4);
    counter += num_bytes;

    num_bytes = min(BUFFER_SIZE, size - counter);
  }

  return counter;
}

int send_end(int client_sock, char *buffer) {
  int send_result = send(client_sock, "END", 4, 0);

  if (send_result == -1) {
    printf("Error : Sending failed.\n");
    return -1;
  } else if (send_result == 0) {
    printf("Error : Client's socket is closed, couldn't send to it.\n");
    return -1;
  } else if (send_result != 4) {
    printf("Error : Client received a corrupted buffer.\n");
    return -1;
  }

  int recv_result = recv(client_sock, buffer, 4, 0);

  if (recv_result < 0) {
    printf("Error : Receiving failed.\n");
    return -1;
  } else if (recv_result == 0) {
    printf("Error : Client's socket is closed, nothing to receive.\n");
    return -1;
  } else if (strcmp(buffer, "ACK") != 0) {
    printf("Error : Server received a corrupted buffer.\n");
    return -1;
  }

  bzero(buffer, (int)(strlen(buffer) + 1));

  recv_result = recv(client_sock, buffer, 4, 0);

  if (recv_result < 0) {
    printf("Error : Receiving failed.\n");
    return -1;
  } else if (recv_result == 0) {
    printf("Error : Client's socket is closed, nothing to receive.\n");
    return -1;
  } else if (strcmp(buffer, "END") != 0) {
    printf("Error : Server received a corrupted buffer.\n");
    return -1;
  }

  send_result = send(client_sock, "ACK", 4, 0);

  if (send_result == -1) {
    printf("Error : Sending failed.\n");
    return -1;
  } else if (send_result == 0) {
    printf("Error : Client's socket is closed, couldn't send to it.\n");
    return -1;
  } else if (send_result != 4) {
    printf("Error : Client received a corrupted buffer.\n");
    return -1;
  }

  bzero(buffer, (int)(strlen(buffer) + 1));
  return 0;
}

int send_again(int client_sock, char *buffer) {
  int send_result = send(client_sock, "AGAIN", 6, 0);

  if (send_result == -1) {
    printf("Error : Sending failed.\n");
    return -1;
  } else if (send_result == 0) {
    printf("Error : Client's socket is closed, couldn't send to it.\n");
    return -1;
  } else if (send_result != 6) {
    printf("Error : Client received a corrupted buffer.\n");
    return -1;
  }

  int recv_result = recv(client_sock, buffer, 4, 0);

  if (recv_result < 0) {
    printf("Error : Receiving failed.\n");
    return -1;
  } else if (recv_result == 0) {
    printf("Error : Client's socket is closed, nothing to receive.\n");
    return -1;
  } else if (strcmp(buffer, "ACK")) {
    printf("Error : Server received a corrupted buffer.\n");
    return -1;
  }

  bzero(buffer, 1);
  return 0;
}

int get_file_size(FILE *fp) {
  int size;
  fseek(fp, 0, SEEK_END);
  size = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  return size;
}

int min(int a, int b) {
  if (a < b) {
    return a;
  } else {
    return b;
  }
}
