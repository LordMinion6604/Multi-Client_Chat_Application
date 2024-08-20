#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

int clients[MAX_CLIENTS];
int client_count = 0;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void broadcast_message(char *message, int sender) {
    pthread_mutex_lock(&clients_mutex);

    for (int i = 0; i < client_count; ++i) {
        if (clients[i] != sender) {
            send(clients[i], message, strlen(message), 0);
        }
    }

    pthread_mutex_unlock(&clients_mutex);
}

void *handle_client(void *client_socket) {
    int sock = *(int *)client_socket;
    char buffer[BUFFER_SIZE];

    while (1) {
        int bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0) {
            break;
        }

        buffer[bytes_received] = '\0';
        broadcast_message(buffer, sock);
    }

    pthread_mutex_lock(&clients_mutex);

    // Remove the client from the list
    for (int i = 0; i < client_count; ++i) {
        if (clients[i] == sock) {
            for (int j = i; j < client_count - 1; ++j) {
                clients[j] = clients[j + 1];
            }
            break;
        }
    }

    client_count--;
    pthread_mutex_unlock(&clients_mutex);

    close(sock);
    free(client_socket);
    pthread_exit(NULL);
}

int main() {
    int server_socket, new_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    // Create server socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Could not create socket");
        return 1;
    }

    // Configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind socket to the specified IP and port
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        return 1;
    }

    // Start listening for incoming connections
    if (listen(server_socket, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        return 1;
    }

    printf("Server listening on port 8080...\n");

    while (1) {
        new_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);
        if (new_socket < 0) {
            perror("Accept failed");
            continue;
        }

        // Add new client to the list
        pthread_mutex_lock(&clients_mutex);
        if (client_count < MAX_CLIENTS) {
            clients[client_count++] = new_socket;
            pthread_mutex_unlock(&clients_mutex);

            // Create a thread to handle the new client
            pthread_t tid;
            int *client_sock = malloc(sizeof(int));
            *client_sock = new_socket;
            pthread_create(&tid, NULL, handle_client, (void *)client_sock);

        } else {
            pthread_mutex_unlock(&clients_mutex);
            close(new_socket);
            printf("Max clients reached. Connection refused.\n");
        }
    }

    return 0;
}
