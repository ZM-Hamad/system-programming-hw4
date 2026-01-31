#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 5555
#define BUFFER_SIZE 4096

/* Global counter for connected clients */
int connected_clients = 0;

/* Mutex to protect the critical section */
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Helper function: receive all data */
ssize_t recv_all(int sockfd, char *buffer, size_t length) {
    size_t total = 0;
    ssize_t bytes;

    while (total < length) {
        bytes = recv(sockfd, buffer + total, length - total, 0);
        if (bytes <= 0)
            return bytes;
        total += bytes;
    }
    return total;
}

/* Helper function: send all data */
ssize_t send_all(int sockfd, const char *buffer, size_t length) {
    size_t total = 0;
    ssize_t bytes;

    while (total < length) {
        bytes = send(sockfd, buffer + total, length - total, 0);
        if (bytes <= 0)
            return bytes;
        total += bytes;
    }
    return total;
}

/* Thread function that handles a single client */
void *client_handler(void *arg) {
    int client_socket = *(int *)arg;
    free(arg);

    char buffer[BUFFER_SIZE];
    ssize_t received;

    /* Critical section: increment client counter */
    pthread_mutex_lock(&clients_mutex);
    connected_clients++;
    printf("Client connected. Active clients: %d\n", connected_clients);
    pthread_mutex_unlock(&clients_mutex);

    while ((received = recv(client_socket, buffer, BUFFER_SIZE, 0)) > 0) {
        /* Convert lowercase letters to uppercase */
        for (ssize_t i = 0; i < received; i++) {
            buffer[i] = toupper((unsigned char)buffer[i]);
        }

        /* Ensure full send */
        if (send_all(client_socket, buffer, received) < 0) {
            perror("send");
            break;
        }
    }

    close(client_socket);

    /* Critical section: decrement client counter */
    pthread_mutex_lock(&clients_mutex);
    connected_clients--;
    printf("Client disconnected. Active clients: %d\n", connected_clients);
    pthread_mutex_unlock(&clients_mutex);

    return NULL;
}

int main() {
    int server_socket, *client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    pthread_t tid;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 10) < 0) {
        perror("listen");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Echo Server is running on port %d...\n", PORT);

    while (1) {
        client_socket = malloc(sizeof(int));
        *client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);
        if (*client_socket < 0) {
            perror("accept");
            free(client_socket);
            continue;
        }

        if (pthread_create(&tid, NULL, client_handler, client_socket) != 0) {
            perror("pthread_create");
            close(*client_socket);
            free(client_socket);
        }

        pthread_detach(tid);
    }

    close(server_socket);
    return 0;
}
