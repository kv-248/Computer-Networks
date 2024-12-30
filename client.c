#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

void* client_task(void* arg) {
    int sock = 0;
    struct sockaddr_in serv_addr;
    const char *hello = "Hello from client";
    char buffer[1024] = {0};
    pthread_t this_id = pthread_self();  
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Thread %lu: Socket creation error\n", this_id);
        pthread_exit(NULL);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8080);
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("Thread %lu: Invalid address/Address not supported\n", this_id);
        close(sock);
        pthread_exit(NULL);
    }
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("Thread %lu: Connection Failed\n", this_id);
        close(sock);
        pthread_exit(NULL);
    }
    send(sock, hello, strlen(hello), 0);
    printf("Thread %lu: Hello message sent\n", this_id);
    int valread = read(sock, buffer, 1024);
    if (valread > 0) {
        printf("Thread %lu: Message received: %s\n", this_id, buffer);
    } else {
        printf("Thread %lu: Failed to read message\n", this_id);
    }
    close(sock);
    pthread_exit(NULL);
}

int main(int argc, char const *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <number of clients>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int num_clients = atoi(argv[1]);
    pthread_t threads[num_clients];
    for (int i = 0; i < num_clients; i++) {
        if (pthread_create(&threads[i], NULL, client_task, NULL) != 0) {
            printf("Error creating thread %d\n", i);
        }
    }
    for (int i = 0; i < num_clients; i++) {
        pthread_join(threads[i], NULL);
    }

    return 0;
}
