#include <arpa/inet.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_BUFFER 1024
#define TENNER 10
#define FIVER 5

// Function prototypes
_Noreturn void *accept_connections(void *server_socket_ptr);
void           *handle_peer_connection(void *socket_ptr);

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

    // Client socket setup
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
        close(peer_socket);    // Close the socket as connection failed
    }
    else
    {
        pthread_t client_thread;
        int      *client_sock_ptr;
        printf("Connected to peer. You can start chatting now!\n");
        client_sock_ptr = (int *)malloc(sizeof(int));
        if(client_sock_ptr == NULL)
        {
            perror("Memory allocation failed");
            exit(EXIT_FAILURE);
        }
        *client_sock_ptr = peer_socket;

        pthread_create(&client_thread, NULL, handle_peer_connection, client_sock_ptr);
        pthread_join(client_thread, NULL);
    }

    pthread_join(accept_thread, NULL);
    close(server_socket);
    return 0;
}

_Noreturn void *accept_connections(void *server_socket_ptr)
{
    int                server_socket = *(int *)server_socket_ptr;
    struct sockaddr_in client_addr;
    socklen_t          client_addr_len = sizeof(client_addr);

    while(1)
    {
        int      *new_sock_ptr = (int *)malloc(sizeof(int));
        pthread_t thread_id;
        if(new_sock_ptr == NULL)
        {
            perror("Memory allocation failed");
            continue;
        }

        *new_sock_ptr = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if(*new_sock_ptr == -1)
        {
            free(new_sock_ptr);
            perror("Accept failed");
            continue;
        }

        printf("Connection accepted. You can start chatting now!\n");

        pthread_create(&thread_id, NULL, handle_peer_connection, new_sock_ptr);
        pthread_detach(thread_id);
    }
}

void *handle_peer_connection(void *socket_ptr)
{
    int  sock = *(int *)socket_ptr;
    char buffer[MAX_BUFFER];
    free(socket_ptr);

    while(1)
    {
        ssize_t bytes_received;
        memset(buffer, 0, MAX_BUFFER);
        bytes_received = read(sock, buffer, MAX_BUFFER - 1);
        if(bytes_received <= 0)
        {
            if(bytes_received == 0)
            {
                printf("Peer disconnected.\n");
            }
            else
            {
                perror("Read error");
            }
            break;
        }
        printf("Peer: %s\n", buffer);

        printf("Enter message (or type 'quit' to exit):\n");
        if(fgets(buffer, MAX_BUFFER, stdin) == NULL)
        {
            perror("Error reading input");
            break;
        }
        if(strncmp(buffer, "quit\n", FIVER) == 0)
        {
            printf("Quitting. Goodbye!\n");
            break;
        }
        if(write(sock, buffer, strlen(buffer)) < 0)
        {
            perror("Write error");
            break;
        }
    }

    close(sock);
    return NULL;
}
