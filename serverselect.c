#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/select.h>
#include <stdlib.h>
#include <errno.h>
#include <dirent.h>

#define PORT 8080
#define BUFFER_SIZE 1024

typedef struct {
    char name[256];
    int pid;
    unsigned long utime;  // user time taken inside the CPU
    unsigned long stime;  // kernel time
} process_info;

// Function to get process information from /proc/[pid]/stat
void get_process_info(int pid, process_info *pinfo) {
    char filepath[BUFFER_SIZE];
    sprintf(filepath, "/proc/%d/stat", pid);

    FILE *file = fopen(filepath, "r");
    if (file) {
        // Reading fields from the file: pid, process name, utime, stime
        fscanf(file, "%d %s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %lu %lu", 
               &pinfo->pid, pinfo->name, &pinfo->utime, &pinfo->stime);
        fclose(file);
    } else {
        pinfo->pid = -1; // Invalid pid
    }
}

// Function to find the top two CPU-consuming processes
void find_top_two_processes(process_info *top_two) {
    DIR *dir = opendir("/proc");
    if (!dir) {
        perror("Could not open /proc directory");
        return;
    }

    struct dirent *entry;
    process_info current;
    
    top_two[0].utime = 0; // Initialize as smallest value
    top_two[1].utime = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR) {
            int pid = atoi(entry->d_name);  // converting folder name to integer
            if (pid > 0) {
                get_process_info(pid, &current);
                unsigned long total_time = current.utime + current.stime;
                
                if (total_time > (top_two[0].utime + top_two[0].stime)) {
                    top_two[1] = top_two[0]; // Second largest becomes the first
                    top_two[0] = current;    // New top process
                } else if (total_time > (top_two[1].utime + top_two[1].stime)) {
                    top_two[1] = current;    // New second top process
                }
            }
        }
    }
    closedir(dir);
}

int main(int argc, char const *argv[]) {
    if (argc < 2) {  // The user should give n- the number of clients
        printf("Usage: %s <number of clients>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int n = atoi(argv[1]);
    int server_fd, new_socket, max_sd, sd;
    int client_sockets[n];  // Array to keep all the client sockets
    struct sockaddr_in address;
    fd_set readfds;
    char buffer[BUFFER_SIZE];

    // Initialize client sockets
    for (int i = 0; i < n; i++) {
        client_sockets[i] = 0;
    }

    // Create server socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Setsockopt failed");
        exit(EXIT_FAILURE);
    }

    // Bind the socket to the address and port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, n) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Listening on port %d\n", PORT);
    int addrlen = sizeof(address);

    while (1) {
        // Clear the socket set
        FD_ZERO(&readfds);

        // Add server socket to set
        FD_SET(server_fd, &readfds);
        max_sd = server_fd;

        // Add client sockets to set
        for (int i = 0; i < n; i++) {
            sd = client_sockets[i];
            if (sd > 0) {
                FD_SET(sd, &readfds);
            }
            if (sd > max_sd) {
                max_sd = sd;
            }
        }

        // Wait for activity on any of the sockets
        int activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
        if ((activity < 0) && (errno != EINTR)) {
            perror("Select error");
        }

        // If something happened on the server socket, it's an incoming connection
        if (FD_ISSET(server_fd, &readfds)) {
            if ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
                perror("Accept failed");
                exit(EXIT_FAILURE);
            }

            // Add new socket to the client_sockets array
            for (int i = 0; i < n; i++) {
                if (client_sockets[i] == 0) {
                    client_sockets[i] = new_socket;
                    break;
                }
            }
        }

        // Check all client sockets
        for (int i = 0; i < n; i++) {
            sd = client_sockets[i];
            if (FD_ISSET(sd, &readfds)) {
                // Check if it was for closing, and also read the incoming message
                int cpu_size_val = read(sd, buffer, BUFFER_SIZE);
                if (cpu_size_val == 0) {
                    // Client disconnected
                    getpeername(sd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
                    close(sd);
                    client_sockets[i] = 0;
                } else {
                    // Process client request
                    buffer[cpu_size_val] = '\0';
                    printf("Message Received: %s\n", buffer);

                    // Find the top two CPU-consuming processes
                    process_info top_two[2];
                    find_top_two_processes(top_two);

                    // Prepare response
                    char response[BUFFER_SIZE];
                    snprintf(response, BUFFER_SIZE,
                        "Top 2 CPU-consuming processes:\n"
                        "1. PID: %d, Name: %s, User Time: %lu, Kernel Time: %lu\n"
                        "2. PID: %d, Name: %s, User Time: %lu, Kernel Time: %lu\n",
                        top_two[0].pid, top_two[0].name, top_two[0].utime, top_two[0].stime,
                        top_two[1].pid, top_two[1].name, top_two[1].utime, top_two[1].stime);

                    // Send the response back to the client
                    if (send(sd, response, strlen(response), 0) < 0) {
                        perror("Could not send statistics to client");
                    } else {
                        printf("Information sent to client.\n");
                    }
                }
            }
        }
    }

    return 0;
}
