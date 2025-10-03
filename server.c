#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#define PORT 8080

void handle_client(int client_sock) {
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    read(client_sock, buffer, sizeof(buffer));
    printf("Received: %s\n", buffer);
    write(client_sock, "Hello from server", 17);
    close(client_sock);
}

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;

    server_sock = socket(AF_INET, SOCK_STREAM, 0);

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
    listen(server_sock, 5);

    //Exercise 1 
    while (1) {
        addr_size = sizeof(client_addr);
        client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addr_size);
    
        pid_t child_pid = fork();
        if (child_pid == 0) {
            close(server_sock); 
            handle_client(client_sock);
            exit(0); 
        } else if (child_pid > 0) {
            close(client_sock); 
            waitpid(-1, NULL, WNOHANG); 
        }
      
    }
    return 0;
}