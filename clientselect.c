#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <number_of_requests>\n", argv[0]);
        return -1;
    }

    int num_requests = atoi(argv[1]);  // Convert the argument to an integer
    if (num_requests <= 0) {
        printf("Please enter a valid number of requests.\n");
        return -1;
    }

    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
    const char *message = "Hello from the client";

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        // 0 sets it to default , tcp for SOCK_STREAM
        perror("Socket creation error");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert address from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        //inet_pton stands for "Internet Protocol Presentation to Network." It converts an IP address from its human-readable text form (presentation format) to the binary form used by network functions
        perror("Invalid address/ Address not supported");
        return -1;
    }

    // Attempt to connect to the server
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection Failed");
        return -1;
    }

    printf("Connected to server. Sending %d automatic messages...\n", num_requests);

    for (int i = 0; i < num_requests; i++) {
        if (send(sock, message, strlen(message), 0) < 0) {
            perror("Send failed");
            break;
        }
        printf("Message %d sent: %s\n", i + 1, message);

        // Clear buffer before receiving new data
        memset(buffer, 0, BUFFER_SIZE);

        // Receive message from the server
        int valread = read(sock, buffer, BUFFER_SIZE);
        if (valread > 0) {
            buffer[valread] = '\0';  // Null-terminate the received string
            printf("Server replied: %s\n", buffer);
        } else if (valread == 0) {
            printf("Server closed the connection\n");
            break;
        } else {
            perror("Read failed");
            break;
        }

        // Optionally add a delay (e.g., sleep) to prevent spamming the server
        // sleep(1);
    }

    close(sock);  // Close the socket
    return 0;
}
