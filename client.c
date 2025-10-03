#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <signal.h>

#define PORT 8080
#define BUFFER_SIZE 1024

volatile sig_atomic_t keep_running = 1;

void handle_signal(int sig) {
    keep_running = 0;
}

int main() {
    int sock;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    char message[BUFFER_SIZE];
    
    // Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        return 1;
    }
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);
    
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connect failed");
        return 1;
    }
    
    printf("Connected to chat server. Type 'exit' to quit.\n");
    
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }
    
    if (pid == 0) {
        // Child process - continuously listen for messages from server
        signal(SIGTERM, handle_signal);
        
        while(keep_running) {
            memset(buffer, 0, BUFFER_SIZE);
            int read_size = read(sock, buffer, BUFFER_SIZE - 1);
            
            if (read_size > 0) {
                buffer[read_size] = '\0';
                printf("\n%s", buffer);
                printf("Enter message: ");
                fflush(stdout);
            } else if (read_size == 0) {
                printf("\nServer disconnected.\n");
                break;
            } else {
                if (keep_running) {
                    perror("Read error");
                }
                break;
            }
        }
        exit(0);
    } else {
        // Parent process - send messages to server
        while(1) {
            printf("Enter message: ");
            fflush(stdout);
            
            if (fgets(message, BUFFER_SIZE, stdin) == NULL) {
                break;
            }
            
            // Remove trailing newline if present
            message[strcspn(message, "\n")] = '\0';
            
            // Add newline back for sending
            strcat(message, "\n");
            
            if (write(sock, message, strlen(message)) < 0) {
                perror("Write failed");
                break;
            }
            
            if (strncmp(message, "exit", 4) == 0) {
                break;
            }
        }
        
        // Clean up child process
        kill(pid, SIGTERM);
        waitpid(pid, NULL, 0);
    }
    
    close(sock);
    return 0;
}