#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <cstring>

#define PORT 8080

void handle_client(int client_sock) {
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    read(client_sock, buffer, sizeof(buffer));
    std::cout << "Received: " << buffer << std::endl;
    write(client_sock, "Hello from C++ server", 22);
    close(client_sock);
}

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;

    server_sock = socket(AF_INET, SOCK_STREAM, 0);

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
    listen(server_sock, 5);

    // Exercise 2
    while (true) {
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