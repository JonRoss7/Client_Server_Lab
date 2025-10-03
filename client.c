// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <unistd.h>
// #include <arpa/inet.h>

// #define PORT 8080

// int main() {
//     int sock;
//     struct sockaddr_in server_addr;
//     char buffer[1024] = "Hello Server";

//     sock = socket(AF_INET, SOCK_STREAM, 0);
//     server_addr.sin_family = AF_INET;
//     server_addr.sin_port = htons(PORT);
//     inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

//     connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
//     send(sock, buffer, strlen(buffer), 0);
//     read(sock, buffer, sizeof(buffer));
//     printf("Server response: %s\n", buffer);
//     close(sock);
//     return 0;
// }

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
    int sock;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    char message[BUFFER_SIZE];

    sock = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connect failed");
        return 1;
    }
    printf("Connected to server. Type 'exit' to quit.\n");

    while(1) {
        printf("Enter message: ");
        
        fgets(message, BUFFER_SIZE, stdin);

       
        write(sock, message, strlen(message));

       
        if (strncmp(message, "exit", 4) == 0) {
            break;
        }

        memset(buffer, 0, BUFFER_SIZE);
        read(sock, buffer, BUFFER_SIZE - 1);
        printf("%s", buffer);
    }

    close(sock);
    printf("Connection closed.\n");
    return 0;
}