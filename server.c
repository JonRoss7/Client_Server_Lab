#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <sys/time.h> 
#include <errno.h>  
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <signal.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 50

struct SharedClientList {
    int count;
    int sockets[MAX_CLIENTS];
};

union semun { int val; };

void sem_lock(int sem_id) {
    struct sembuf sb = {0, -1, 0};
    semop(sem_id, &sb, 1);
}

void sem_unlock(int sem_id) {
    struct sembuf sb = {0, 1, 0};
    semop(sem_id, &sb, 1);
}

int shm_id;
int sem_id;

void cleanup_ipc(int sig) {
    printf("\nServer shutting down. Cleaning up IPC resources...\n");
    shmctl(shm_id, IPC_RMID, NULL);
    semctl(sem_id, 0, IPC_RMID);
    exit(0);
}

void handle_client(int client_sock) {
    char buffer[BUFFER_SIZE];
    int read_size;
    
    struct SharedClientList* client_list = (struct SharedClientList*)shmat(shm_id, NULL, 0);

    while(1) { 
        fd_set fds;
        struct timeval timeout;
        FD_ZERO(&fds);
        FD_SET(client_sock, &fds);
        timeout.tv_sec = 60;
        timeout.tv_usec = 0;

        int activity = select(client_sock + 1, &fds, NULL, NULL, &timeout);

        if ((activity < 0) && (errno != EINTR)) { 
            break; 
        }
        if (activity == 0) { 
            printf("Client %d timed out.\n", client_sock); 
            break; 
        }

        if ((read_size = read(client_sock, buffer, BUFFER_SIZE - 1)) > 0) {
            buffer[read_size] = '\0';
            buffer[strcspn(buffer, "\n")] = 0;

            if (strcmp(buffer, "exit") == 0) { 
                break; 
            }

            printf("Received from client %d: %s\n", client_sock, buffer);

            // Broadcast to all OTHER clients
            sem_lock(sem_id);
            char broadcast_msg[BUFFER_SIZE + 50];
            snprintf(broadcast_msg, sizeof(broadcast_msg), "Client %d says: %s\n", client_sock, buffer);
            
            for (int i = 0; i < client_list->count; i++) {
                if (client_list->sockets[i] != client_sock) {
                    write(client_list->sockets[i], broadcast_msg, strlen(broadcast_msg));
                }
            }
            sem_unlock(sem_id);

            // Echo back to sender
            char echo_buffer[BUFFER_SIZE + 10];
            snprintf(echo_buffer, sizeof(echo_buffer), "Echo: %s\n", buffer);
            write(client_sock, echo_buffer, strlen(echo_buffer));
        } else {
            break;
        }
    }
    
    // Remove client from shared list
    sem_lock(sem_id);
    for (int i = 0; i < client_list->count; i++) {
        if (client_list->sockets[i] == client_sock) {
            for (int j = i; j < client_list->count - 1; j++) {
                client_list->sockets[j] = client_list->sockets[j + 1];
            }
            client_list->count--;
            break;
        }
    }
    printf("Client %d disconnected. Total clients: %d\n", client_sock, client_list->count);
    sem_unlock(sem_id);
    shmdt(client_list);
    
    close(client_sock);
}

int main() {
    // Create shared memory for client list
    shm_id = shmget(IPC_PRIVATE, sizeof(struct SharedClientList), 0666 | IPC_CREAT);
    struct SharedClientList* client_list = (struct SharedClientList*)shmat(shm_id, NULL, 0);
    client_list->count = 0;
    shmdt(client_list);

    // Create semaphore
    sem_id = semget(IPC_PRIVATE, 1, 0666 | IPC_CREAT);
    union semun su;
    su.val = 1;
    semctl(sem_id, 0, SETVAL, su);

    signal(SIGINT, cleanup_ipc);

    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("Socket creation failed");
        return 1;
    }

    // Allow address reuse
    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        return 1;
    }
    
    if (listen(server_sock, 5) < 0) {
        perror("Listen failed");
        return 1;
    }

    printf("Server listening on port %d...\n", PORT);

    while (1) {
        addr_size = sizeof(client_addr);
        client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addr_size);
        
        if (client_sock < 0) {
            perror("Accept failed");
            continue;
        }

        // Add client to shared list BEFORE forking
        client_list = (struct SharedClientList*)shmat(shm_id, NULL, 0);
        sem_lock(sem_id);
        client_list->sockets[client_list->count++] = client_sock;
        printf("Client %d connected. Total clients: %d\n", client_sock, client_list->count);
        sem_unlock(sem_id);
        shmdt(client_list);
    
        pid_t child_pid = fork();
        if (child_pid == 0) {
            // Child process
            close(server_sock); 
            handle_client(client_sock);
            exit(0); 
        } else if (child_pid > 0) {
            // Parent process
            close(client_sock); 
            waitpid(-1, NULL, WNOHANG); 
        } else {
            perror("Fork failed");
        }
    }
    
    close(server_sock);
    return 0;
}