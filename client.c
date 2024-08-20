#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define BUFFER_SIZE 1024

void *receive_messages(void *socket_desc) {
    int sock = *(int *)socket_desc;
    char buffer[BUFFER_SIZE];

    while (1) {
        int bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0) {
            break;
        }

        buffer[bytes_received] = '\0';
        printf("%s\n", buffer);
    }

    pthread_exit(NULL);
}

int main() {
    int client_socket;
    struct sockaddr_in server_addr;
    pthread_t recv_thread;
    char message[BUFFER_SIZE];

    // Create client socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("Could not create socket");
        return 1;
    }

    // Configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Connect to server
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        return 1;
    }

    printf("Connected to the server...\n");

    // Start a thread to receive messages from the server
    pthread_create(&recv_thread, NULL, receive_messages, &client_socket);

    while (1) {
        fgets(message, BUFFER_SIZE, stdin);
        send(client_socket, message, strlen(message), 0);
    }

    close(client_socket);
    return 0;
}
