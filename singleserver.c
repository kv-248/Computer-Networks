#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <ctype.h>

#define PORT 8080
#define BUFFER_SIZE 1024

#define BUFFER_SIZE 1024
typedef struct {
    char name[256];
    int pid;
    long unsigned int utime; 
    long unsigned int stime; 
} process_info;

void get_process_info(int pid, process_info *pinfo) {
    char filepath[BUFFER_SIZE];
    sprintf(filepath, "/proc/%d/stat", pid);
    FILE *file = fopen(filepath, "r");
    
    if (file) {
        fscanf(file, "%d %s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %lu %lu", 
               &pinfo->pid, pinfo->name, &pinfo->utime, &pinfo->stime);
        fclose(file);
    } else {
        pinfo->pid = -1; 
    }
}

void find_top_two_processes(process_info *top_two) {
    DIR * dir = opendir("/proc");
    struct dirent *entry;

    process_info current;
    top_two[0].utime = 0;
    top_two[1].utime = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR) {
            int pid = atoi(entry->d_name);
            if (pid > 0) {
                get_process_info(pid, &current);
                unsigned long total_time = current.utime + current.stime;
                if (total_time > (top_two[0].utime + top_two[0].stime)) {
                    top_two[1] = top_two[0];
                    top_two[0] = current;
                } else if (total_time > (top_two[1].utime + top_two[1].stime)) {
                    top_two[1] = current;
                }
            }
        }
    }
    closedir(dir);
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};
    
    // Create socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET; // IPv4
    address.sin_addr.s_addr = INADDR_ANY; // Any IP address
    address.sin_port = htons(PORT); // Port number

    // Bind the socket to the address and port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Start listening for incoming connections
    if (listen(server_fd, 3) < 0) {
        perror("Listen");
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port %d\n", PORT);

    // Loop to accept and handle clients sequentially
    while (1) {
        // Accept a connection
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("Accept");
            exit(EXIT_FAILURE);
        }

        printf("Client connected\n");

        // Get top two CPU processes and send to client
        process_info top_two[2];
        find_top_two_processes(top_two);

        // Format the process information into the buffer
        snprintf(buffer, BUFFER_SIZE, "Top CPU Processes:\n1. PID: %d, Name: %s, UTime: %lu, STime: %lu\n2. PID: %d, Name: %s, UTime: %lu, STime: %lu\n",
                 top_two[0].pid, top_two[0].name, top_two[0].utime, top_two[0].stime,
                 top_two[1].pid, top_two[1].name, top_two[1].utime, top_two[1].stime);

        // Send the data to the client
        send(new_socket, buffer, strlen(buffer), 0);

        // Close the client socket after serving the client
        close(new_socket);

        printf("Client disconnected\n");
    }
    
    // Close the server socket
    close(server_fd);
    
    return 0;
}
