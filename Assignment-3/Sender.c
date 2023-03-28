#include <stdio.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <math.h>

#define server_port 5060 // The port that the server listens
#define buffer_size 1024 // The size of the file we want to send.

// **FUNCTION HEADERS**:

int sendFIN(int clientSock, char *buffer);
int sendFileSize(int clientSock, char *buffer, int size);
int getKey(int clientSock, char *clientKey, char *serverKey);
int sendFile(FILE *fp, int clientSock, int size, int counter, char buffer[buffer_size]);
int sendAGAIN(int clientSock, char *buffer);
int sendEND(int clientSock, char *buffer);
int file_size(FILE *fp);
int min(int a, int b);

int main()
{

    signal(SIGPIPE, SIG_IGN); // Helps preventing crashing when closing the socket later on.

    int temp = 0; // Setting a temporary variable to help check for errors throughout the program.

    int listenSock = -1;
    listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // Setting the listening socket
    if (listenSock == -1)
    {
        printf("Error : Listen socket creation failed.\n"); // if the creation of the socket failed,
        return 0;                                           // print the error and exit main.
    }

    int reuse = 1;
    temp = setsockopt(listenSock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)); // uses a previously chosen socketopt if there is one.
    if (temp < 0)
    {
        printf("Error : Failed to set congestion control algorithm to reno.\n"); // If the socket's reuse failed,
        return 0;                                                                // print the error and exit main.
    }

    struct sockaddr_in server_address;                  // Using the imported struct of a socket address using the IPv4 module.
    memset(&server_address, 0, sizeof(server_address)); // Resetting it to default values.

    server_address.sin_family = AF_INET;          // Server address is type IPv4.
    server_address.sin_addr.s_addr = INADDR_ANY;  // Get any address that tries to connect.
    server_address.sin_port = htons(server_port); // Set the server port to the defined 'server_port'.

    temp = bind(listenSock, (struct sockaddr *)&server_address, sizeof(server_address)); // Bind the socket to a port and IP.
    if (temp == -1)
    {
        printf("Error: Binding failed.\n"); // If the binding failed,
        close(listenSock);                  // print the corresponding error, close the socket and exit main.
        return -1;
    }

    temp = listen(listenSock, 3); // Start listening, and set the max queue for awaiting client to 3.
    if (temp == -1)
    {
        printf("Error: Listening failed."); // If listen failed,
        close(listenSock);                  // print the corresponding error, close the socket and exit main.
        return -1;
    }

    // Accept and incoming connection
    struct sockaddr_in client_address;                 // Using the imported struct of a socket address using the IPv4 module.
    socklen_t client_add_len = sizeof(client_address); // Setting the size of the address to match what's needed.

    while (1)
    {
        printf("Waiting for a connection...\n");

        memset(&client_address, 0, sizeof(client_address));                                       // Resetting address struct to default values.
        client_add_len = sizeof(client_address);                                                  // Setting the size of the address to match what's needed.
        int clientSock = accept(listenSock, (struct sockaddr *)&client_address, &client_add_len); // Accepting the clients request to connect.

        if (clientSock == -1)
        {
            printf("Error: Accepting a client failed."); // If accept failed,
            close(listenSock);                           // print an error, close the socket and exit main.
            return -1;
        }

        printf("Connected to client!\n");

        FILE *fp = fopen("send.txt", "r"); // Opening the file to send.
        if (fp == NULL)
        {
            printf("File open error\n"); // If the file failed to open,
            close(clientSock);           // print an error, close the sockets and exit main.
            close(listenSock);
            return -1;
        }

        char buffer[buffer_size] = {0}; // Setting the buffer size to the defined 'buffer_size'.
        int size = file_size(fp);       // Getting the size of the file, and returning the pointer 'fp' to the beginning of the file.
        int counter = 0;                // Setting a counter to keep track of how many bytes have been sent.

        char clientKey[10] = {0};      // Setting the client key buffer.
        char serverKey[10] = {0};      // Setting the server key buffer to the size of the key.
        int key = 1714 ^ 6521;         // Calculating the key.
        sprintf(serverKey, "%d", key); // Storing the key in the server key buffer.

        char message[1] = {0}; // Setting the message buffer responsible for checking if the server wants to send the file again.

        // ######################### Sending the size of the file: ###############################

        printf("Sending size of the file...\n");

        temp = sendFileSize(clientSock, buffer, size);
        if (temp == -1)
        {
            close(clientSock); // If the sending of the size failed,
            close(listenSock); // close the sockets and exit main.
            return -1;
        }

        printf("Size of the file sent successfully!\n");

        while (1)
        {
            // ############### Setting the congestion control algorithm to reno: #####################

            if (setsockopt(listenSock, IPPROTO_TCP, TCP_CONGESTION, "reno", 6) < 0)
            {
                printf("Error : Failed to set congestion control algorithm to reno.\n"); // If the setting of the congestion control algorithm failed, print an error and exit main.
                close(clientSock);
                close(listenSock);
                return -1;
            }

            printf("CC algorithm set to reno.\n");

            // ####################### Sending the 1st part of the file: #############################

            printf("Sending first part of the file...\n");

            counter = sendFile(fp, clientSock, size / 2, counter, buffer);

            if (counter == -1)
            {
                close(clientSock); // If the sending of the first part of the file failed,
                close(listenSock); // close the sockets and exit main.
                return -1;
            }

            printf("First part of the file sent successfully!\n");

            // ################ Asking the client for the key and checking if it matches: #############

            printf("Asking client for key...\n");

            temp = getKey(clientSock, clientKey, serverKey);

            if (temp == -1)
            {
                close(clientSock); // If the receiving of the key failed,
                close(listenSock); // print an error and exit main.
                return -1;
            }

            printf("Keys match!\n");

            // ################ Setting the congestion control algorithm to cubic: ####################

            if (setsockopt(listenSock, IPPROTO_TCP, TCP_CONGESTION, "cubic", 6) < 0)
            {
                printf("Error : Failed to set congestion control algorithm to reno.\n");
                close(clientSock); // If the setting of the congestion control algorithm failed,
                close(listenSock); // close the sockets and exit main.
                return -1;
            }

            printf("CC algorithm set to cubic.\n");

            // ######################## Sending the 2nd part of the file: #############################

            printf("Sending second part of the file...\n");

            counter = sendFile(fp, clientSock, size, counter, buffer); // Sending the 2nd part of the file.

            if (counter != size)
            {
                printf("Error: File size didn't match, sending failed.\n"); // If the sending of the second part of the file failed,
                close(clientSock);                                          // If the sending of the second part of the file failed,
                close(listenSock);                                          // close the sockets and exit main.
                return -1;
            }

            printf("Second part of the file sent successfully!\n");

            // ############## Sending the client that the server is done sending the file: ##############

            printf("Letting the client know we finished sending the file...\n");

            temp = sendFIN(clientSock, buffer);

            if (temp == -1)
            {
                close(clientSock); // If the sending of the fin packet failed,
                close(listenSock); // close the sockets and exit main.
                return -1;
            }

            printf("Client acknowledged!\n");

            // ################ Asking sender's permission to send the file again: ######################

            printf("Do you want to send the file again? If so - enter 'y'. If not, enter anything else. ");

            scanf("%s", message);

            if (strcmp(message, "y") == 0) // If the sender wants to send the file again, send the file again.
            {
                counter = 0;            // Resetting the counter to 0.
                fseek(fp, 0, SEEK_SET); // Returning the pointer 'fp' to the beginning of the file.

                temp = sendAGAIN(clientSock, buffer); // Sending the client that the server wants to send the file again.

                if (temp == -1)
                {
                    close(clientSock); // If the sending of the again packet failed,
                    close(listenSock); // close the sockets and exit main.
                    return -1;
                }
            }

            else // If the sender doesn't want to send the file again, break the loop.
            {
                break;
            }
        }

        printf("Asking the client to close connection...\n");

        temp = sendEND(clientSock, buffer); // Asking the client to close the connection.

        if (temp == -1)
        {
            close(clientSock); // If the sending of the end packet failed, print an error and exit main.
            close(listenSock); // If the sending of the end packet failed, exit main.
            return -1;
        }

        printf("Client closed the connection!\n");

        // Finishing up:

        fclose(fp);        // Closing the file.
        close(clientSock); // Closing the client socket.
    }

    close(listenSock);

    printf("\n");

    return 0;
}

// **THE FUNCTIONS** :

// sendFIN(): lets the client know that the server finished sending the file, and gets the client's ACK in return.

int sendFIN(int clientSock, char *buffer)
{
    int sendRequest = send(clientSock, "FIN", 4, 0); // Sending the buffer to the client.

    if (sendRequest == -1)
    {
        printf("Error : Sending failed.\n"); // If the send failed, print an error and exit main.
        return -1;
    }

    else if (sendRequest == 0)
    {
        printf("Error : Client's socket is closed, couldn't send to it.\n"); // If the send failed, print an error and exit main.
        return -1;
    }

    else if (sendRequest != 4)
    {
        printf("Error : Client received a corrupted buffer.\n"); // If the send failed, print an error and exit main.
        return -1;
    }

    // If the program reaches here, then the FIN was sent successfully.

    //_____Receiving the client's ACK:_____//

    int recvResult = recv(clientSock, buffer, 4, 0); // Receiving the client's response.

    if (recvResult < 0)
    {
        printf("Error : Receiving failed.\n"); // If the receiving failed, print an error and exit main.
        return -1;
    }

    else if (recvResult == 0)
    {
        printf("Error : Client's socket is closed, nothing to receive.\n"); // If the receiving failed, print an error and exit main.
        return -1;
    }

    else if (strcmp(buffer, "ACK")) // Checking if the bytes send and the bytes received are equal.
    {
        printf("Error : Server received a corrupted buffer.\n"); // If they aren't, print an error and exit main.
        return -1;
    }

    // if the program reaches here, the FIN ACK was sent successfully.

    bzero(buffer, 1); // Resetting the buffer to default values.

    return 0;
}

// sendFileSize() This function sends the client the size of the file he's about to receive, and gets the client's ACK in return.

int sendFileSize(int clientSock, char *buffer, int size)
{
    sprintf(buffer, "%d", size); // Converting the size of the file to a string and saving it in the buffer.

    int sendSize = send(clientSock, buffer, strlen(buffer) + 1, 0); // Sending the buffer to the client.

    if (sendSize == -1)
    {
        printf("Error : Sending dropped.\n"); // If the send failed, print an error and exit main.
        return -1;
    }

    else if (sendSize == 0)
    {
        printf("Error : Client's socket is closed, couldn't send to it.\n"); // If the send failed, print an error and exit main.
        return -1;
    }

    else if (sendSize != strlen(buffer) + 1) // Checking if the bytes send and the bytes received are equal.
    {
        printf("Error : Server sent a corrupted buffer.\n"); // If they aren't, print an error and exit main.
        return -1;
    }

    // If the program reaches here, then the file size was sent successfully.

    bzero(buffer, (int)(strlen(buffer) + 1)); // Resetting the buffer to default values.

    //_____Receiving the client's ACK:_____//

    int recvResult = recv(clientSock, buffer, 4, 0); // Receiving the client's response.

    if (recvResult < 0)
    {
        printf("Error : Receiving failed.\n"); // If the receiving failed, print an error and exit main.
        return -1;
    }

    else if (recvResult == 0)
    {
        printf("Error : Client's socket is closed, nothing to receive.\n"); // If the receiving failed, print an error and exit main.
        return -1;
    }

    else if (strcmp(buffer, "ACK") != 0) // Checking if the message received is indeed an ACK.
    {
        printf("Error : Server received a corrupted buffer.\n"); // If they aren't, print an error and exit main.
        return -1;
    }

    bzero(buffer, 4); // Resetting the buffer to default values.
}

// getKey() asks the client for a key and receives it//

int getKey(int clientSock, char *clientKey, char *serverKey)
{
    int sendRequest = send(clientSock, "SEND KEY", 9, 0); // Sending the buffer to the client.

    if (sendRequest == -1)
    {
        printf("Error : Sending faied.\n"); // If the send failed, print an error and exit main.
        return -1;
    }

    else if (sendRequest == 0)
    {
        printf("Error : Client's socket is closed, couldn't send to it.\n"); // If the send failed, print an error and exit main.
        return -1;
    }
    else if (sendRequest != 9)
    {
        printf("Error : Client received a corrupted buffer.\n"); // If the send failed, print an error and exit main.
        return -1;
    }

    // If the program reaches here, then the request was sent successfully.

    // Receiving the client's key:

    int recvKey = recv(clientSock, clientKey, 5, 0); // Receiving the client's response.

    if (recvKey < 0)
    {
        printf("Error : Receiving failed.\n"); // If the receive failed, print an error and exit main.
        return -1;
    }

    else if (recvKey == 0)
    {
        printf("Error : Client's socket is closed, nothing to receive.\n"); // If the receive failed, print an error and exit main.
        return -1;
    }

    else if (recvKey != 5) // Checking if the bytes send and the bytes received are equal.
    {
        printf("Error : Server received a corrupted buffer.\n"); // If they aren't, print an error and exit main.
        return -1;
    }

    else if ((strcmp(serverKey, clientKey)) != 0) // Comparing the server and client's keys.
    {
        printf("Error : Keys don't match.\n"); // If the keys don't match, print an error and exit main.
        return -1;
    }

    // If the program reaches here, then the keys match.

    int sendOK = send(clientSock, "OK", 3, 0); // Sending the buffer to the client.

    if (sendOK == -1)
    {
        printf("Error : Sending failed.\n"); // If the send failed, print an error and exit main.
        return -1;
    }

    else if (sendOK == 0)
    {
        printf("Error : Client's socket is closed, couldn't send to it.\n"); // If the send failed, print an error and exit main.
        return -1;
    }
    else if (sendOK != 3)
    {
        printf("Error : Client received a corrupted buffer.\n"); // If the send failed, print an error and exit main.
        return -1;
    }

    bzero(clientKey, 10); // Resetting the clientKey to default values.

    return 0;
}

//_____sendFile function: sending the file to the client and receiving an ACK from the client_____//

int sendFile(FILE *fp, int clientSock, int size, int counter, char buffer[buffer_size])
{
    int numofbytes = min(buffer_size, size - counter); // Getting the minimum of the buffer size and the remaining bytes to send.

    while (counter < size && fread(buffer, numofbytes, 1, fp) != 0)
    {

        //_____Sending the buffer to the client:_____//

        int sendResult = send(clientSock, buffer, numofbytes, 0); // Sending the buffer to the client.

        if (sendResult == -1)
        {
            printf("Error : Sending failed.\n"); // If the send failed, print an error and exit main.
            return -1;
        }

        else if (sendResult == 0)
        {
            printf("Error : Client's socket is closed, couldn't send to it.\n"); // If the send failed, print an error and exit main.
            return -1;
        }

        else if (sendResult != numofbytes)
        {
            printf("Error : Client received a corrupted buffer.\n"); // If the send failed, print an error and exit main.
            return -1;
        }

        // if the program reaches here, the 'i' buffer was sent successfully.

        bzero(buffer, numofbytes); // Resetting the buffer.

        //_____Receiving the client's ACK:_____//

        int recvResult = recv(clientSock, buffer, 4, 0); // Receiving the client's response.

        if (recvResult < 0)
        {
            printf("Error : Receiving failed.\n"); // If the receive failed, print an error and exit main.
            return -1;
        }

        else if (recvResult == 0)
        {
            printf("Error : Client's socket is closed, nothing to receive.\n"); // If the receive failed, print an error and exit main.
            return -1;
        }

        else if (strcmp(buffer, "ACK") != 0) // Checking if the bytes send and the bytes received are equal.
        {
            printf("Error : Server received a corrupted buffer.\n"); // If they aren't, print an error and exit main.
            return -1;
        }

        // if the program reaches here, the 'i' ACK was sent successfully.

        bzero(buffer, 4);      // Resetting the buffer.
        counter += numofbytes; // Adding the number of bytes sent to the counter.

        numofbytes = min(buffer_size, size - counter); // Adjusting the number of bytes to send.
    }

    return counter;
}

//_____sendEnd function: sending the client that the server wants to end the connection_____//

int sendEND(int clientSock, char *buffer)
{

    // Sending the client a request to end the connection:

    int sendRequest = send(clientSock, "END", 4, 0); // Sending END message to the client.

    if (sendRequest == -1)
    {
        printf("Error : Sending failed.\n"); // If the send failed, print an error and exit main.
        return -1;
    }

    else if (sendRequest == 0)
    {
        printf("Error : Client's socket is closed, couldn't send to it.\n"); // If the send failed, print an error and exit main.
        return -1;
    }

    else if (sendRequest != 4)
    {
        printf("Error : Client received a corrupted buffer.\n"); // If the send failed, print an error and exit main.
        return -1;
    }

    // Receiving the client's ACK for closing the connection:

    int recvResult = recv(clientSock, buffer, 4, 0); // Receiving the client's response.

    if (recvResult < 0)
    {
        printf("Error : Receiving failed.\n"); // If the receiving failed, print an error and exit main.
        return -1;
    }

    else if (recvResult == 0)
    {
        printf("Error : Client's socket is closed, nothing to receive.\n"); // If the receiving failed, print an error and exit main.
        return -1;
    }

    else if (strcmp(buffer, "ACK") != 0) // Checking if the bytes send and the bytes received are equal.
    {
        printf("Error : Server received a corrupted buffer.\n"); // If they aren't, print an error and exit main.
        return -1;
    }

    bzero(buffer, (int)(strlen(buffer) + 1)); // Resetting the buffer to default values.

    // Since both parties need to send an 'END' to end the connection,
    // The server will be receiving the client's request to end the connection:

    recvResult = recv(clientSock, buffer, 4, 0); // Receiving the client's request to end.

    if (recvResult < 0)
    {
        printf("Error : Receiving failed.\n"); // If the receiving failed, print an error and exit main.
        return -1;
    }

    else if (recvResult == 0)
    {
        printf("Error : Client's socket is closed, nothing to receive.\n"); // If the receiving failed, print an error and exit main.
        return -1;
    }

    else if (strcmp(buffer, "END") != 0) // Checking if the bytes send and the bytes received are equal.
    {
        printf("Error : Server received a corrupted buffer.\n"); // If they aren't, print an error and exit main.
        return -1;
    }

    // Sending the client an ACK for closing the connection:

    sendRequest = send(clientSock, "ACK", 4, 0); // Sending ACK message to the client.

    if (sendRequest == -1)
    {
        printf("Error : Sending failed.\n"); // If the send failed, print an error and exit main.
        return -1;
    }

    else if (sendRequest == 0)
    {
        printf("Error : Client's socket is closed, couldn't send to it.\n"); // If the send failed, print an error and exit main.
        return -1;
    }

    else if (sendRequest != 4)
    {
        printf("Error : Client received a corrupted buffer.\n"); // If the send failed, print an error and exit main.
        return -1;
    }

    bzero(buffer, (int)(strlen(buffer) + 1)); // Resetting the buffer to default values.
    return 0;
}

//_____sendAGAIN function: sending the client that the server wants to send the file again the connection_____//

int sendAGAIN(int clientSock, char *buffer)
{

    int sendRequest = send(clientSock, "AGAIN", 6, 0); // Sending END message to the client.

    if (sendRequest == -1)
    {
        printf("Error : Sending failed.\n"); // If the send failed, print an error and exit main.
        return -1;
    }

    else if (sendRequest == 0)
    {
        printf("Error : Client's socket is closed, couldn't send to it.\n"); // If the send failed, print an error and exit main.
        return -1;
    }

    else if (sendRequest != 6)
    {
        printf("Error : Client received a corrupted buffer.\n"); // If the send failed, print an error and exit main.
        return -1;
    }

    // If the program reaches here, then END message was sent to the client.

    //_____Receiving the client's ENDACK:_____//

    int recvResult = recv(clientSock, buffer, 4, 0); // Receiving the client's response.

    if (recvResult < 0)
    {
        printf("Error : Receiving failed.\n"); // If the receiving failed, print an error and exit main.
        return -1;
    }

    else if (recvResult == 0)
    {
        printf("Error : Client's socket is closed, nothing to receive.\n"); // If the receiving failed, print an error and exit main.
        return -1;
    }

    else if (strcmp(buffer, "ACK")) // Checking if the bytes send and the bytes received are equal.
    {
        printf("Error : Server received a corrupted buffer.\n"); // If they aren't, print an error and exit main.
        return -1;
    }

    // if the program reaches here, the END ACK was received successfully.

    bzero(buffer, 1); // Resetting the buffer to default values.
    return 0;
}

//_____file_size function: returns the size of the file_____//

int file_size(FILE *fp)
{
    int size;
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    return size;
}

//_____min function: returns the minimum of two numbers_____//

int min(int a, int b)
{
    if (a < b)
    {
        return a;
    }
    else
    {
        return b;
    }
}
