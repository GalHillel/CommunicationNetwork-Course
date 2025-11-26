#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <netinet/tcp.h>

#define SERVER_PORT 5060
#define SERVER_IP "127.0.0.1"
#define BUFFER_SIZE 1024
#define CC_ALGO_1 "reno"
#define CC_ALGO_2 "cubic"

// **Function Headers**:

/**
 * Receives the file size from the server.
 * @param sock The socket descriptor.
 * @param buffer The buffer to use for receiving.
 * @param ack The ACK message to send.
 * @return The file size, or -1 on error.
 */
int recv_file_size(int sock, char *buffer, char *ack);

/**
 * Sends the key to the server.
 * @param sock The socket descriptor.
 * @param buffer The buffer to use for sending.
 * @return 0 on success, -1 on error.
 */
int send_key(int sock, char *buffer);

/**
 * Sends an ACK message to the server.
 * @param sock The socket descriptor.
 * @param ack The ACK message to send.
 * @return 0 on success, -1 on error.
 */
int send_ack(int sock, char *ack);

/**
 * Writes a chunk of data to the file.
 * @param fp The file pointer.
 * @param sock The socket descriptor.
 * @param chunk_size The size of the chunk.
 * @param buffer The buffer containing the chunk.
 * @param ack The ACK message to send.
 * @return 0 on success, -1 on error.
 */
int write_chunk(FILE *fp, int sock, int chunk_size, char *buffer, char *ack);

/**
 * Sends an END message to the server.
 * @param sock The socket descriptor.
 * @param buffer The buffer to use for receiving the ACK.
 * @return 0 on success, -1 on error.
 */
int send_end(int sock, char *buffer);

int main()
{
    int temp = 0;
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (sock == -1)
    {
        printf("Error : Listen socket creation failed.\n");
        return -1;
    }

    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(SERVER_PORT);
    temp = inet_pton(AF_INET, (const char *)SERVER_IP, &server_address.sin_addr);

    if (temp <= 0)
    {
        printf("Error: inet_pton() failed.\n");
        return -1;
    }

    int connect_result = connect(sock, (struct sockaddr *)&server_address, sizeof(server_address));
    if (connect_result == -1)
    {
        printf("Error: Connecting to server failed.\n");
        close(sock);
        return -1;
    }

    printf("Connected to server!\n");

    char buffer[BUFFER_SIZE] = {0};
    char *ack_msg = "ACK";

    printf("Receiving file size...\n");

    int size = recv_file_size(sock, buffer, ack_msg);

    if (size == -1)
    {
        printf("Error : File size receiving failed.\n");
        close(sock);
        return -1;
    }

    int counter = 0;   // The current bit of the file that will be written.
    int chunk_size = 0; // The size of the chunk received from the server.

    if (size < 0)
    {
        printf("Error : File size receiving failed.\n");
        close(sock);
        return -1;
    }

    printf("File size received successfully!\n");

    // We now declare an array to store the time it took to receive each half of the file.
    // The **even** indices will store the time it took to receive the first half (meaning in cc algorithm reno).
    // The **odd** indices will store the time it took to receive the second half (meaning in cc algorithm cubic).

    double time_measurements[1000] = {0};
    struct timeval start, end;
    long sec, micsec;
    int current = 0;
    int flag = 1;

    while (1)
    {
        FILE *fp = fopen("recv.txt", "w");
        if (fp == NULL)
        {
            printf("Error : File opening failed.\n");
            close(sock);
            return -1;
        }

        // Setting the congestion control algorithm to reno for the receival of the first half of the file.
        if (setsockopt(sock, IPPROTO_TCP, TCP_CONGESTION, CC_ALGO_1, 6) < 0)
        {
            printf("Error : Failed to set congestion control algorithm to reno.\n");
            close(sock);
            return -1;
        }

        printf("CC algorithm set to %s.\n", CC_ALGO_1);
        printf("Receiving the first part of the file...\n");

        while (1)
        {
            if (flag == 1)
            {
                gettimeofday(&start, NULL);
                printf("Set start!");
                flag = 0;
            }

            temp = recv(sock, buffer, BUFFER_SIZE, 0);

            // If the counter + num of bytes we received equals to size/2 or size, we have received half of the size of the file.
            if ((counter + temp == size / 2) && strcmp(buffer, "SEND KEY") != 0)
            {
                gettimeofday(&end, NULL);
                printf("Set end!");
                sec = end.tv_sec - start.tv_sec;
                micsec = end.tv_usec - start.tv_usec;
                time_measurements[current] = sec + micsec * (1e-6);
                current++;
                flag = 1;
            }

            if ((counter + temp == size) && strcmp(buffer, "FIN") != 0)
            {
                gettimeofday(&end, NULL);
                printf("Set end!");
                sec = end.tv_sec - start.tv_sec;
                micsec = end.tv_usec - start.tv_usec;
                time_measurements[current] = sec + micsec * (1e-6);
                current++;
                flag = 1;
            }

            if (temp == -1)
            {
                printf("Error : Receive failed.\n");
                close(sock);
                return -1;
            }
            else if (size == 0)
            {
                printf("Error : Server's socket is closed, couldn't receive anything.\n");
                close(sock);
                return -1;
            }
            else if (strcmp(buffer, "SEND KEY") == 0)
            {
                printf("First part of the file received successfully!\n");
                printf("Sending key...\n");

                temp = send_key(sock, buffer);

                if (temp == -1)
                {
                    printf("Error : Key sending failed.\n");
                    close(sock);
                    return -1;
                }

                printf("Keys matched!\n");

                // If the counter is half the size of the file, change the congestion control algorithm to cubic.
                if (setsockopt(sock, IPPROTO_TCP, TCP_CONGESTION, CC_ALGO_2, 6) < 0)
                {
                    printf("Error : Failed to set congestion control algorithm to cubic.\n");
                    close(sock);
                    return -1;
                }

                printf("CC algorithm set to %s.\n", CC_ALGO_2);
                printf("Receiving the second part of the file...\n");
            }
            else if (strcmp(buffer, "FIN") == 0)
            {
                temp = send_ack(sock, ack_msg);

                if (temp == -1)
                {
                    close(sock);
                    return -1;
                }

                printf("Second part of the file received successfully!\n");
                break;
            }
            else
            {
                chunk_size = temp;
                temp = write_chunk(fp, sock, chunk_size, buffer, ack_msg);

                if (temp == -1)
                {
                    printf("Error : Chunk writing failed.\n");
                    close(sock);
                    return -1;
                }

                counter += chunk_size;

                if (counter > size)
                {
                    printf("Error : Received more bytes than the server's file size.\n");
                    close(sock);
                    return -1;
                }
            }
        }

        printf("File received successfully!\n");

        temp = recv(sock, buffer, BUFFER_SIZE, 0);

        if (temp == -1)
        {
            printf("Error : Receive failed.\n");
            close(sock);
            return -1;
        }
        else if (size == 0)
        {
            printf("Error : Server's socket is closed, couldn't receive anything.\n");
            close(sock);
            return -1;
        }
        else if (strcmp(buffer, "AGAIN") == 0)
        {
            temp = send_ack(sock, ack_msg);

            if (temp == -1)
            {
                close(sock);
                return -1;
            }

            printf("Server wishes to send the file again, deleting the file and preparing to receive it again...\n");

            fseek(fp, 0, SEEK_SET);
            fclose(fp);
            int rmv = remove("recv.txt");
            counter = 0;

            if (rmv != 0)
            {
                printf("Error : File removal failed.\n");
                close(sock);
                return -1;
            }

            printf("File deleted successfully!\n");
        }
        else if (strcmp(buffer, "END") == 0)
        {
            temp = send_ack(sock, ack_msg);

            if (temp == -1)
            {
                close(sock);
                return -1;
            }

            printf("Server wishes to end the connection, sending END message and closing the socket...\n");

            temp = send_end(sock, buffer);

            if (temp == -1)
            {
                close(sock);
                return -1;
            }

            break;
        }
        else
        {
            printf("Error : Received unexpected data.\n");
            close(sock);
            return -1;
        }
    }

    close(sock);
    printf("Socket closed, goodbye!\n");

    printf("\n");
    printf("Time it took to receive each iteration of 1st half of the file (in %s cc protocol):\n", CC_ALGO_1);
    printf("\n");

    int ind = 1;
    double evensum = 0;
    int i;

    for (i = 0; i < current; i += 2)
    {
        printf("Iteration %d: %f seconds.\n", ind, time_measurements[i]);
        evensum += time_measurements[i];
        ind++;
    }

    double evenavg = evensum / (ind - 1);

    printf("\n");
    printf("Average time for %s cc protocol: %f seconds.\n", CC_ALGO_1, evenavg);
    printf("\n");

    printf("Time it took to receive each iteration of the 2nd half of the file (in %s cc protocol):\n", CC_ALGO_2);

    ind = 1;
    double oddsum = 0;

    for (i = 1; i < current; i += 2)
    {
        printf("Iteration %d: %f seconds.\n", ind, time_measurements[i]);
        oddsum += time_measurements[i];
        ind++;
    }

    double oddavg = oddsum / (ind - 1);

    printf("\n");
    printf("Average time for %s cc protocol: %f seconds.\n", CC_ALGO_2, oddavg);
    printf("\n");

    return 0;
}

// ########################## THE FUNCTIONS: #############################

int recv_file_size(int sock, char *buffer, char *ack)
{
    int bytes = recv(sock, buffer, BUFFER_SIZE, 0);

    if (bytes == -1)
    {
        printf("Error : Receive failed.\n");
        return -1;
    }
    else if (bytes == 0)
    {
        printf("Error : Server's socket is closed, couldn't receive anything.\n");
        return -1;
    }

    int ack_result = send_ack(sock, ack);

    if (ack_result == -1)
    {
        return -1;
    }

    char **end;
    long val = strtol(buffer, end, 10);
    int size = (int)val;

    bzero(buffer, (int)(strlen(buffer) + 1));

    return size;
}

int send_key(int sock, char *buffer)
{
    int key = 1714 ^ 6521;
    sprintf(buffer, "%d", key);

    int send_result = send(sock, buffer, (int)(strlen(buffer) + 1), 0);

    if (send_result == -1)
    {
        printf("Error : Sending failed.\n");
        return -1;
    }
    else if (send_result == 0)
    {
        printf("Error : Server's socket is closed, couldn't send to it.\n");
        return -1;
    }
    else if (send_result != (int)(strlen(buffer) + 1))
    {
        printf("Error : Server received a corrupted buffer.\n");
        return -1;
    }

    bzero(buffer, (int)(strlen(buffer) + 1));

    int recv_result = recv(sock, buffer, 3, 0);

    if (recv_result == -1)
    {
        printf("Error : Receive failed.\n");
        close(sock);
        return -1;
    }
    else if (recv_result == 0)
    {
        printf("Error : Server's socket is closed, couldn't receive anything.\n");
        close(sock);
        return -1;
    }
    else if (strcmp(buffer, "OK") != 0)
    {
        printf("Error : Key doesn't match the server's.\n");
        close(sock);
        return -1;
    }

    bzero(buffer, (int)(strlen(buffer) + 1));

    return 0;
}

int send_ack(int sock, char *ack)
{
    int send_result = send(sock, ack, 4, 0);

    if (send_result == -1)
    {
        printf("Error : Receive failed.\n");
        return -1;
    }
    else if (send_result == 0)
    {
        printf("Error : Client's socket is closed, couldn't receive anything.\n");
        return -1;
    }

    if (send_result != 4)
    {
        printf("Error : Server received a corrupted buffer.\n");
        return -1;
    }
    return 0;
}

int write_chunk(FILE *fp, int sock, int chunk_size, char *buffer, char *ack)
{
    fwrite(buffer, chunk_size, 1, fp);

    int send_result = send(sock, ack, 4, 0);

    if (send_result == -1)
    {
        printf("Error : Sending failed.");
        return -1;
    }
    else if (send_result == 0)
    {
        printf("Error : Server's socket is closed, couldn't send to it.");
        return -1;
    }
    else if (send_result != 4)
    {
        printf("Error : Server received a corrupted buffer.");
        return -1;
    }

    bzero(buffer, chunk_size);

    return 0;
}

int send_end(int sock, char *buffer)
{
    int send_result = send(sock, "END", 4, 0);

    if (send_result == -1)
    {
        printf("Error : Receive failed.\n");
        return -1;
    }
    else if (send_result == 0)
    {
        printf("Error : Client's socket is closed, couldn't receive anything.\n");
        return -1;
    }

    if (send_result != 4)
    {
        printf("Error : Server received a corrupted buffer.\n");
        return -1;
    }

    int recv_result = recv(sock, buffer, BUFFER_SIZE, 0);

    if (recv_result == -1)
    {
        printf("Error : Receive failed.\n");
        close(sock);
        return -1;
    }
    else if (recv_result == 0)
    {
        printf("Error : Server's socket is closed, couldn't receive anything.\n");
        close(sock);
        return -1;
    }
    else if (strcmp(buffer, "ACK") != 0)
    {
        printf("Error : Server's ACK not received propperly.\n");
        close(sock);
        return -1;
    }

    bzero(buffer, (int)(strlen(buffer) + 1));

    return 0;
}
