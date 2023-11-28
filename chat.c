#include <arpa/inet.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_BUFFER 1024
#define TENNER 10

void *receive_messages(void *socket);

int main(int argc, char const *argv[])
{
    const char        *peer_ip;
    uint16_t           peer_port;
    int                server_socket;
    int                client_socket;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    socklen_t          client_addr_len;
    long               port_long;
    char              *endptr;
    pthread_t          thread_id;
    int               *new_sock;
    struct sockaddr_in peer_addr;
    int                peer_socket;
    char               buffer[MAX_BUFFER];
    int                yes;

    port_long = strtol(argv[2], &endptr, TENNER);    // Base TENNER
    // Check for errors: e.g., no digits found or value out of range
    if(endptr == argv[2] || *endptr != '\0' || port_long < 0 || port_long > UINT16_MAX)
    {
        fprintf(stderr, "Invalid port number\n");
        exit(EXIT_FAILURE);
    }

    if(argc != 3)
    {
        fprintf(stderr, "Usage: %s <peer_ip> <peer_port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    peer_port       = (uint16_t)port_long;
    client_addr_len = sizeof(client_addr);
    peer_ip         = argv[1];

    // Create server socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(server_socket == -1)
    {
        perror("Server socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set the SO_REUSEADDR socket option
    yes = 1;
    if(setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1)
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // Now proceed to bind the server socket
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port        = htons(peer_port);

    if(bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen on server socket
    if(listen(server_socket, 1) == -1)
    {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Waiting for peer connection...\n");

    // Accept client connections in a new thread
    client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
    if(client_socket == -1)
    {
        perror("Accept failed");
        exit(EXIT_FAILURE);
    }

    new_sock  = (int *)malloc(sizeof(int));
    *new_sock = client_socket;
    if(pthread_create(&thread_id, NULL, receive_messages, (void *)new_sock) != 0)
    {
        perror("Thread creation failed");
        exit(EXIT_FAILURE);
    }

    // Connect to peer

    peer_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(peer_socket == -1)
    {
        perror("Peer socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&peer_addr, 0, sizeof(peer_addr));
    peer_addr.sin_family = AF_INET;
    peer_addr.sin_port   = htons(peer_port);
    inet_pton(AF_INET, peer_ip, &peer_addr.sin_addr);

    if(connect(peer_socket, (struct sockaddr *)&peer_addr, sizeof(peer_addr)) == -1)
    {
        perror("Connection to peer failed");
        exit(EXIT_FAILURE);
    }

    printf("Connected to peer. You can start chatting now!\n");

    // Send messages to peer

    while(1)
    {
        memset(buffer, 0, sizeof(buffer));
        fgets(buffer, MAX_BUFFER, stdin);

        if(write(peer_socket, buffer, strlen(buffer)) < 0)
        {
            perror("Write to peer failed");
            break;
        }
    }

    close(peer_socket);
    close(server_socket);
    return 0;
}

void *receive_messages(void *socket)
{
    int  client_socket = *(int *)socket;
    char buffer[MAX_BUFFER];

    while(1)
    {
        ssize_t bytes_received;
        memset(buffer, 0, sizeof(buffer));
        bytes_received = read(client_socket, buffer, sizeof(buffer) - 1);    // Declare here
        if(bytes_received <= 0)
        {
            perror("Read failed or connection closed");
            break;
        }
        printf("Peer: %s\n", buffer);
    }

    close(client_socket);
    free(socket);
    return NULL;
}
