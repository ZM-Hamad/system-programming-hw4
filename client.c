#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 5555
#define SERVER_IP "127.0.0.1"
#define BUFFER_SIZE 4096
#define CLIENT_THREADS 5

void *client_thread(void *arg) {
    int sock;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE] = "hello from client thread\n";
    char response[BUFFER_SIZE];

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return NULL;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(sock);
        return NULL;
    }

    send(sock, buffer, strlen(buffer), 0);
    recv(sock, response, BUFFER_SIZE, 0);

    printf("Response: %s", response);

    close(sock);
    return NULL;
}

int main() {
    pthread_t threads[CLIENT_THREADS];

    for (int i = 0; i < CLIENT_THREADS; i++) {
        pthread_create(&threads[i], NULL, client_thread, NULL);
    }

    for (int i = 0; i < CLIENT_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    return 0;
}
