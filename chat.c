#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
/*
 * Alexander Gibbison
 * Roy (Xavier) Pimentel
 * Project: TCP peer-to-peer chat client
 *
 * */
#define TEN 10
#define THOU 1024

int main(int argc, const char *argv[])
{
    const char        *server_ip;
    long               port;
    int                client_socket;
    struct sockaddr_in server_address;
    ssize_t            bytes_sent;
    ssize_t            bytes_received;    // Change the type to ssize_t
    const char        *message;
    char               buffer[THOU];    // Buffer to store received data

    if(argc != 4)
    {
        fprintf(stderr, "Usage: %s <server_ip> <server_port> \"message\"\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Check if the message is empty
    if(strlen(argv[3]) == 0)
    {
        fprintf(stderr, "Error: The message is empty.\n");
        exit(EXIT_FAILURE);
    }

    // Declarations
    server_ip = argv[1];
    errno     = 0;    // Initialize errno before the call
    port      = strtol(argv[2], NULL, TEN);
    message   = argv[3];

    // Create a socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(client_socket == -1)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Initialize server_address struct
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port   = htons((uint16_t)port);
    inet_pton(AF_INET, server_ip, &(server_address.sin_addr));

    // Connect to the server
    if(connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1)
    {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    printf("Connected to server\n");

    // Send data to the server
    bytes_sent = write(client_socket, message, strlen(message));
    if(bytes_sent == -1)
    {
        perror("Write failed");
        exit(EXIT_FAILURE);
    }

    // Receive data from the server
    bytes_received = read(client_socket, buffer, sizeof(buffer));
    if(bytes_received == -1)
    {
        perror("Read failed");
        exit(EXIT_FAILURE);
    }

    // Null-terminate the received data to print it
    buffer[bytes_received] = '\0';

    printf("Received data from server: %s\n", buffer);

    // Close the socket
    close(client_socket);
    return 0;
}
