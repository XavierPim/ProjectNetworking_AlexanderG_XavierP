#include <arpa/inet.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_BUFFER 1024
#define TENNER 10

void *accept_connections(void *server_socket_ptr);
void *receive_messages(void *socket);
void *send_messages(void *socket);

int main(int argc, char const *argv[])
{
    const char        *peer_ip;
    uint16_t           peer_port;
    int                server_socket;
    int                peer_socket;
    struct sockaddr_in server_addr;
    struct sockaddr_in peer_addr;
    long               port_long;
    char              *endptr;
    pthread_t          accept_thread;
    pthread_t          send_thread;
    char               buffer[MAX_BUFFER];

    if(argc != 3)
    {
        fprintf(stderr, "Usage: %s <peer_ip> <peer_port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    peer_ip   = argv[1];
    port_long = strtol(argv[2], &endptr, TENNER);
    if(endptr == argv[2] || *endptr != '\0' || port_long < 0 || port_long > UINT16_MAX)
    {
        fprintf(stderr, "Invalid port number\n");
        exit(EXIT_FAILURE);
    }

    peer_port = (uint16_t)port_long;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(server_socket == -1)
    {
        perror("Server socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port        = htons(peer_port);

    if(bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if(listen(server_socket, 1) == -1)
    {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server is waiting for a connection...\n");

    pthread_create(&accept_thread, NULL, accept_connections, &server_socket);

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
        close(peer_socket);
        peer_socket = -1;
    }
    else
    {
        printf("Connected to peer. You can start chatting now!\n");
    }

    pthread_create(&send_thread, NULL, send_messages, &peer_socket);

    while(1)
    {
        ssize_t bytes_received;
        memset(buffer, 0, sizeof(buffer));
        bytes_received = read(peer_socket, buffer, sizeof(buffer) - 1);

        if(bytes_received <= 0)
        {
            perror("Read failed or connection closed");
            break;
        }

        printf("Peer: %s\n", buffer);
    }

    if(peer_socket != -1)
    {
        close(peer_socket);
    }
    pthread_join(accept_thread, NULL);
    close(server_socket);
    return 0;
}

void *accept_connections(void *server_socket_ptr)
{
    int                server_socket = *(int *)server_socket_ptr;
    struct sockaddr_in client_addr;
    socklen_t          client_addr_len = sizeof(client_addr);
    int                new_sock;
    pthread_t          recv_thread;

    new_sock = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
    if(new_sock == -1)
    {
        perror("Accept failed");
        exit(EXIT_FAILURE);
    }

    printf("Connection accepted. You can start chatting now!\n");

    pthread_create(&recv_thread, NULL, receive_messages, &new_sock);
    pthread_detach(recv_thread);

    while(1)
    {
        char buffer[MAX_BUFFER];
        memset(buffer, 0, sizeof(buffer));
        fgets(buffer, MAX_BUFFER, stdin);

        if(write(new_sock, buffer, strlen(buffer)) < 0)
        {
            perror("Write to peer failed");
            break;
        }
    }
    close(new_sock);
    return NULL;
}

void *receive_messages(void *socket)
{
    int  client_socket = *(int *)socket;
    char buffer[MAX_BUFFER];

    while(1)
    {
        ssize_t bytes_received;
        memset(buffer, 0, sizeof(buffer));

        bytes_received = read(client_socket, buffer, sizeof(buffer) - 1);

        if(bytes_received <= 0)
        {
            perror("Read failed or connection closed");
            break;
        }

        printf("Peer: %s\n", buffer);
    }

    close(client_socket);
    return NULL;
}

void *send_messages(void *socket)
{
    int  peer_socket = *(int *)socket;
    char buffer[MAX_BUFFER];

    while(1)
    {
        ssize_t bytes_received;
        memset(buffer, 0, sizeof(buffer));
        bytes_received = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);

        if(bytes_received <= 0)
        {
            perror("Read from stdin failed");
            break;
        }

        if(write(peer_socket, buffer, strlen(buffer)) < 0)
        {
            perror("Write to peer failed");
            break;
        }
    }

    return NULL;
}
