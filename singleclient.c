#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

int main(int argc, char const *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <number of clients>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int num_clients = atoi(argv[1]);
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
    const char *hello = "Hello from client";

    // Set server address properties
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8080);

    // Convert IPv4 address from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("Invalid address/Address not supported\n");
        return EXIT_FAILURE;
    }

    for (int i = 0; i < num_clients; i++) {
        int sock = 0;
        
        // Create socket
        if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            printf("Client %d: Socket creation error\n", i + 1);
            continue;
        }

        // Connect to the server
        if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
            printf("Client %d: Connection failed\n", i + 1);
            close(sock);
            continue;
        }

        // Send message to the server
        send(sock, hello, strlen(hello), 0);
        printf("Client %d: Hello message sent\n", i + 1);

        // Read response from the server
        int valread = read(sock, buffer, BUFFER_SIZE);
        if (valread > 0) {
            printf("Client %d: Message received from server: %s\n", i + 1, buffer);
        } else {
            printf("Client %d: Failed to read message\n", i + 1);
        }

        // Close the socket
        close(sock);
    }

    return 0;
}
