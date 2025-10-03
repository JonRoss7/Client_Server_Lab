#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <cstring>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <csignal>
#include <fstream>
#include <cerrno>
#include <ctime>

#define PORT 8080

int shm_id = -1;
int* active_clients_ptr = nullptr;

void log_error(const std::string& message) {
    std::ofstream log_file("server_errors.log", std::ios_base::app);
    if (!log_file.is_open()) {
        std::cerr << "Fatal Error: Could not open log file." << std::endl;
        return;
    }
    time_t now = time(0);
    char* dt = ctime(&now);
    dt[strlen(dt) - 1] = '\0'; 

    log_file << "[" << dt << "] " << message << ": " << strerror(errno) << std::endl;
}

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
    shm_id = shmget(IPC_PRIVATE, sizeof(int), 0666 | IPC_CREAT);
    
    if (shm_id == -1) {
        log_error("shmget failed");
        exit(EXIT_FAILURE);
    }

    active_clients_ptr = (int*) shmat(shm_id, NULL, 0);
    if (active_clients_ptr == (void*) -1) {
        log_error("shmat failed");
        exit(EXIT_FAILURE);
    }
    *active_clients_ptr = 0; 

    signal(SIGINT, cleanup_shm);
    
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == -1) {
        log_error("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        log_error("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_sock, 5) < 0) {
        log_error("Listen failed");
        exit(EXIT_FAILURE);
    }

    while (true) {
        addr_size = sizeof(client_addr);
        client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addr_size);

        if (client_sock < 0) {
            log_error("Accept failed");
            continue;
        }

        pid_t child_pid = fork();
        
        if (child_pid < 0) {
            log_error("Fork failed");
            close(client_sock);
            continue;
        }
        
        if (child_pid == 0) { 
            close(server_sock);
            

            int* child_counter_ptr = (int*) shmat(shm_id, NULL, 0);
            
            if (child_counter_ptr == (void*) -1) {
                log_error("shmat failed in child");
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