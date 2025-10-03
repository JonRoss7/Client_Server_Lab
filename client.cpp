#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>

#define PORT 8080

int main() {
    int sock;
    struct sockaddr_in server_addr;
    char buffer[1024] = "Hello C++ Server";

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

    // Send the initial message
    send(sock, buffer, strlen(buffer), 0);
    
    // Clear the buffer to prepare for the server's response
    memset(buffer, 0, sizeof(buffer));

    // Read the server's response into the empty buffer
    read(sock, buffer, sizeof(buffer));
    std::cout << "Server response: " << buffer << std::endl;
    
    close(sock);
    return 0;
}