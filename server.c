#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <fcntl.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

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

void* handle_client(void* socket_desc) {
    int new_socket = *(int*)socket_desc;
    char buffer[BUFFER_SIZE] = {0};
    int errread = read(new_socket, buffer, BUFFER_SIZE);
    if (errread < 0) {
        printf("Error reading from socket\n");
    } else {
        printf("Message received: %s\n", buffer);
    }
    process_info top_two[2];
    find_top_two_processes(top_two);
    char response[BUFFER_SIZE];
    sprintf(response, 
        "Top 2 CPU-consuming processes:\n"
        "1. PID: %d, Name: %s, User Time: %lu, Kernel Time: %lu\n"
        "2. PID: %d, Name: %s, User Time: %lu, Kernel Time: %lu\n",
        top_two[0].pid, top_two[0].name, top_two[0].utime, top_two[0].stime,
        top_two[1].pid, top_two[1].name, top_two[1].utime, top_two[1].stime
    );
    int errsend = send(new_socket, response, strlen(response), 0);
    if (errsend < 0) {
        printf("Error sending message\n");
    } else {
        printf("Process info sent to client\n");
    }
    close(new_socket); 
    free(socket_desc); 
    return NULL;
}


int main(int argc, char const *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <number of clients>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int backlog = atoi(argv[1]);  
    int server_fd ;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        printf("Socket creation failed\n");
        return EXIT_FAILURE;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        printf("Bind failed\n");
        close(server_fd);
        return EXIT_FAILURE;
    }
    if (listen(server_fd, backlog) < 0) {
        printf("Listen failed\n");
        close(server_fd);
        return EXIT_FAILURE;
    }

    printf("Server listening on port 8080...\n");
    int i = 0 ; 
    while (1) {
        int new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        if (new_socket < 0) {
            printf("Accept failed\n");
            continue; 
        }
        int* new_sock = malloc(sizeof(int));
        if (new_sock == NULL) {
            printf("Memory allocation failed\n");
            close(new_socket);
            continue;
        }
        *new_sock = new_socket;
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_client, (void*)new_sock) < 0) {
            printf("Could not create thread\n");
            free(new_sock); 
            close(new_socket);
            continue;
        }
        pthread_detach(thread_id);
        i ++ ;
        if(i == backlog) {
            break;
        }
    }

    close(server_fd);
    return 0;
}
