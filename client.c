#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8080
#define BUFFER_SIZE 1024

void *receive_messages(void *socket_desc) {
    int sock = *(int*)socket_desc;
    char buffer[BUFFER_SIZE];
    ssize_t read_size;

    while ((read_size = recv(sock, buffer, BUFFER_SIZE - 1, 0)) > 0) {
        buffer[read_size] = '\0';
        printf("%s", buffer);
    }

    if (read_size == 0) {
        puts("Server disconnected");
        exit(1);
    } else if (read_size == -1) {
        perror("recv failed");
    }

    return 0;
}

int main() {
    int sock;
    struct sockaddr_in server;
    char message[BUFFER_SIZE], name[30], formatted_message[BUFFER_SIZE+31];
    pthread_t receive_thread;

    // Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Could not create socket");
        return 1;
    }
    puts("Socket created");

    // Set up server address
    server.sin_addr.s_addr = inet_addr("127.0.0.1");  // Server IP address
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);

    // Connect to server
    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("Connect failed. Error");
        return 1;
    }
    puts("Connected to server\n");
    
    // Prompt for user name
    printf("Enter your name: ");
    fgets(name, 30, stdin);
    name[strcspn(name, "\n")] = 0; // Remove newline character

    // Create a thread for receiving messages
    if (pthread_create(&receive_thread, NULL, receive_messages, (void*) &sock) < 0) {
        perror("Could not create thread for receiving messages");
        return 1;
    }
    
    // Send message to server so the server tell everyone that the client joined the chat
    printf("Enter 'exit' to end connection.\n\n");
    char m1[100];      
    strcpy(m1, name);
    strcat(m1, " enter the chat\n");
    if (send(sock, m1, strlen(m1), 0) < 0) {
        puts("Send failed");
        return 1;
    }
    
    // Loop: Send messages to server
    while (1) {
        //printf("Enter message: ");
        fgets(message, BUFFER_SIZE, stdin);

        if (strncmp(message, "exit", 4) == 0) {
            char m[100];      
            strcpy(m, name);
            strcat(m, " left the chat\n");
            if (send(sock, m, strlen(m), 0) < 0) {
                puts("Send failed");
                return 1;
            }
            break;
        }

        // Format message to include name
        snprintf(formatted_message, BUFFER_SIZE+31, "%s: %s", name, message);

        // Send formatted message to server
        if (send(sock, formatted_message, strlen(formatted_message), 0) < 0) {
            puts("Send failed");
            return 1;
        }
    }

    // Clean up
    close(sock);
    return 0;
}

