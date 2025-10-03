// #include <iostream>
// #include <unistd.h>
// #include <sys/socket.h>
// #include <netinet/in.h>
// #include <sys/wait.h>
// #include <cstring>

// #define PORT 8080

// void handle_client(int client_sock) {
//     char buffer[1024];
//     memset(buffer, 0, sizeof(buffer));
//     read(client_sock, buffer, sizeof(buffer));
//     std::cout << "Received: " << buffer << std::endl;
//     write(client_sock, "Hello from C++ server", 22);
//     close(client_sock);
// }

// int main() {
//     int server_sock, client_sock;
//     struct sockaddr_in server_addr, client_addr;
//     socklen_t addr_size;

//     server_sock = socket(AF_INET, SOCK_STREAM, 0);

//     server_addr.sin_family = AF_INET;
//     server_addr.sin_port = htons(PORT);
//     server_addr.sin_addr.s_addr = INADDR_ANY;

//     bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
//     listen(server_sock, 5);

//     // Exercise 2
//     while (true) {
//         addr_size = sizeof(client_addr);
//         client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addr_size);

//         pid_t child_pid = fork();
        
//         if (child_pid == 0) {
            
//             close(server_sock);
//             handle_client(client_sock);
//             exit(0);
//         } else if (child_pid > 0) {
            
//             close(client_sock);
//             waitpid(-1, NULL, WNOHANG);
//         }
        
//     }
//     return 0;
// }

#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <cstring>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <csignal>


#define PORT 8080
#define SHM_KEY 5678 

int shm_id = -1;
int* active_clients_ptr = nullptr;


void cleanup_shm(int sig) {
    std::cout << "\nServer shutting down. Cleaning up shared memory..." << std::endl;
    if (active_clients_ptr) {
        shmdt(active_clients_ptr);
    }
    if (shm_id != -1) {
        shmctl(shm_id, IPC_RMID, NULL);
    }
    exit(0);
}


void handle_client(int client_sock, int* client_counter) {
    
    (*client_counter)++;
    std::cout << "Client connected. Active clients: " << *client_counter << std::endl;

    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    read(client_sock, buffer, sizeof(buffer));
    std::cout << "Received: " << buffer << std::endl;
    write(client_sock, "Hello from C++ server", 22);
    
    
    (*client_counter)--;
    std::cout << "Client disconnected. Active clients: " << *client_counter << std::endl;

    close(client_sock);
}

int main() {
    
    shm_id = shmget(SHM_KEY, sizeof(int), 0666 | IPC_CREAT);
    if (shm_id == -1) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    // Attach the shared memory segment to the parent process
    active_clients_ptr = (int*) shmat(shm_id, NULL, 0);
    if (active_clients_ptr == (void*) -1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }
    *active_clients_ptr = 0; // Initialize the counter to zero

    
    signal(SIGINT, cleanup_shm);
    

    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;

    server_sock = socket(AF_INET, SOCK_STREAM, 0);

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
    listen(server_sock, 5);
    std::cout << "C++ Server (Exercise 4) is listening on port " << PORT << "..." << std::endl;

    while (true) {
        addr_size = sizeof(client_addr);
        client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addr_size);

        pid_t child_pid = fork();
        
        if (child_pid == 0) { 
            close(server_sock);
            
            
            int* child_counter_ptr = (int*) shmat(shm_id, NULL, 0);
            if (child_counter_ptr == (void*) -1) {
                perror("shmat in child");
                exit(EXIT_FAILURE);
            }

            handle_client(client_sock, child_counter_ptr);

            
            shmdt(child_counter_ptr);
         

            exit(0);
        } else if (child_pid > 0) { 
            close(client_sock);
            waitpid(-1, NULL, WNOHANG);
        }
    }
    
    cleanup_shm(0); 
    return 0;
}