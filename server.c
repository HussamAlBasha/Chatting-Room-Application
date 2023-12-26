//This header file is part of the C Standard Library. It provides functionalities for input and output operations.
//ex: printf()
#include <stdio.h>

//This header file includes functions involving memory allocation, process control, conversions, and others.
//ex: exit()
#include <stdlib.h>

//This header file provides access to the POSIX operating system API
#include <unistd.h>

//This header file is part of the C Standard Library and provides functions for dealing with strings and arrays of characters.
// ex: strcpy, strcat, strlen, strncmp, strcspn
#include <string.h>

//This header file includes definitions of structures needed for sockets and functions for socket API,
//which is used for network communication. 
#include <sys/socket.h>

//This header file contains constants and structures needed for internet domain addresses.
// ex: struct sockaddr_in
#include <netinet/in.h>

//This header file contains functions for internet operations.
#include <arpa/inet.h>

//This header file is used for working with POSIX threads
#include <pthread.h>

//#include <sys/socket.h> include:
// socket(): For creating a new socket.
// bind(): For binding a socket to an address.
// listen(): For listening for connections on a socket.
// accept(): For accepting a connection on a socket.
// connect(): For initiating a connection on a socket.
// send(), recv(): For sending and receiving data on a socket.
// setsockopt(), getsockopt(): For setting and getting options on sockets.
// Structures like sockaddr, sockaddr_in for handling socket addresses.

#define PORT 8080
#define MAX_CLIENTS 100
#define BUFFER_SIZE 1024

int client_sockets[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void broadcast_message(int sender, const char *message) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] != 0 && client_sockets[i] != sender) {
            send(client_sockets[i], message, strlen(message), 0);
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void *client_handler(void *socket_desc) {
    int sock = *(int*)socket_desc;
    char buffer[BUFFER_SIZE];
    ssize_t read_size;

    while ((read_size = recv(sock, buffer, BUFFER_SIZE - 1, 0)) > 0) {
        buffer[read_size] = '\0';
        broadcast_message(sock, buffer);
    }

    if (read_size == 0) {
        puts("Client disconnected");
        fflush(stdout);
    } else if (read_size == -1) {
        perror("recv failed");
    }

    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] == sock) {
            client_sockets[i] = 0;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);

    close(sock);
    free(socket_desc);
    return 0;
}

int main() {
    int server_fd, new_socket, *new_sock;
    struct sockaddr_in server, client;
    socklen_t client_size = sizeof(struct sockaddr_in);


    // Create Server socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Could not create socket");
        return 1;
    }
    puts("Socket created");

    // Set up server address
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);

    // Bind the socket with IP
    if (bind(server_fd, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("bind failed");
        return 1;
    }
    puts("Bind done");

    // Start Listining
    // 3 is the maximum length of the queue for pending connections
    listen(server_fd, 3);
    puts("Waiting for incoming connections...");


    // The loop continues accepting new connections as long as accept
    // successfully returns a new socket descriptor for each incoming connection.
    while ((new_socket = accept(server_fd, (struct sockaddr *)&client, &client_size))) {
        printf("Connection accepted from %s:%d\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));

        pthread_t thread_id;
        new_sock = malloc(sizeof(int));
        *new_sock = new_socket;

        // Updating the Client Sockets Array
        
        // The mutex clients_mutex is used to lock the critical section where the client_sockets array is updated.
        // This prevents race conditions when multiple threads try to update the array simultaneously.
        pthread_mutex_lock(&clients_mutex);
        for (int i = 0; i < MAX_CLIENTS; i++) {
            //The new socket is added to the first empty slot in the client_sockets array
            if (client_sockets[i] == 0) {
                client_sockets[i] = new_socket;
                break;
            }
        }
        pthread_mutex_unlock(&clients_mutex);

        // For each new connection, a new thread is created
        if (pthread_create(&thread_id, NULL, client_handler, (void*) new_sock) < 0) {
            perror("could not create thread");
            return 1;
        }
    }

    if (new_socket < 0) {
        perror("accept failed");
        return 1;
    }

    return 0;
}