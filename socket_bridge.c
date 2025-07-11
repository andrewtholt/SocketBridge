#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include <time.h>

#define MAX_MSG_SIZE 1024
#define INPUT_QUEUE_KEY 1234
#define OUTPUT_QUEUE_KEY 5678

// Message structure for SysV message queues
struct msg_buffer {
    long msg_type;
    char msg_text[MAX_MSG_SIZE];
};

// Global variables
int sockfd = -1;
int input_msgid = -1;
int output_msgid = -1;
volatile int running = 1;

// Function prototypes
void cleanup(void);
void signal_handler(int sig);
void* socket_reader_thread(void* arg);
void* msgq_reader_thread(void* arg);
int setup_socket(const char* server_ip, int port);
int setup_message_queues(void);

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server_ip> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char* server_ip = argv[1];
    int port = atoi(argv[2]);

    // Setup signal handler
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Setup cleanup function
    atexit(cleanup);

    // Setup message queues
    if (setup_message_queues() != 0) {
        fprintf(stderr, "Failed to setup message queues\n");
        exit(EXIT_FAILURE);
    }

    // Setup socket connection
    if (setup_socket(server_ip, port) != 0) {
        fprintf(stderr, "Failed to connect to server\n");
        exit(EXIT_FAILURE);
    }

    printf("Connected to server %s:%d\n", server_ip, port);
    printf("Input message queue ID: %d\n", input_msgid);
    printf("Output message queue ID: %d\n", output_msgid);

    // Create threads
    pthread_t socket_thread, msgq_thread;
    
    if (pthread_create(&socket_thread, NULL, socket_reader_thread, NULL) != 0) {
        perror("Failed to create socket reader thread");
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&msgq_thread, NULL, msgq_reader_thread, NULL) != 0) {
        perror("Failed to create message queue reader thread");
        exit(EXIT_FAILURE);
    }

    // Wait for threads to complete
    pthread_join(socket_thread, NULL);
    pthread_join(msgq_thread, NULL);

    return 0;
}

int setup_socket(const char* server_ip, int port) {
    struct sockaddr_in server_addr;

    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        return -1;
    }

    // Setup server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("Invalid address");
        return -1;
    }

    // Connect to server
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        return -1;
    }

    return 0;
}

int setup_message_queues(void) {
    // Create or get input message queue
    input_msgid = msgget(INPUT_QUEUE_KEY, IPC_CREAT | 0666);
    if (input_msgid == -1) {
        perror("Input message queue creation failed");
        return -1;
    }

    // Create or get output message queue
    output_msgid = msgget(OUTPUT_QUEUE_KEY, IPC_CREAT | 0666);
    if (output_msgid == -1) {
        perror("Output message queue creation failed");
        return -1;
    }

    return 0;
}

void* socket_reader_thread(void* arg) {
    (void)arg; // Suppress unused parameter warning
    struct msg_buffer msg;
    char buffer[MAX_MSG_SIZE];
    ssize_t bytes_received;

    while (running) {
        // Read from socket
        bytes_received = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
        
        if (bytes_received <= 0) {
            if (bytes_received == 0) {
                printf("Server disconnected\n");
            } else {
                perror("Socket receive error");
            }
            running = 0;
            break;
        }

        // Null-terminate the received data
        buffer[bytes_received] = '\0';

        // Prepare message for output queue
        msg.msg_type = 1;
        strncpy(msg.msg_text, buffer, MAX_MSG_SIZE - 1);
        msg.msg_text[MAX_MSG_SIZE - 1] = '\0';

        // Send to output message queue
        if (msgsnd(output_msgid, &msg, strlen(msg.msg_text) + 1, IPC_NOWAIT) == -1) {
            if (errno != EAGAIN) {
                perror("Failed to send message to output queue");
            }
        } else {
            printf("Received from server and sent to output queue: %s\n", msg.msg_text);
        }
    }

    return NULL;
}

void* msgq_reader_thread(void* arg) {
    (void)arg; // Suppress unused parameter warning
    struct msg_buffer msg;
    ssize_t bytes_sent;

    while (running) {
        // Read from input message queue
        if (msgrcv(input_msgid, &msg, MAX_MSG_SIZE, 0, IPC_NOWAIT) == -1) {
            if (errno == ENOMSG) {
                // No message available, sleep briefly
                struct timespec ts = {0, 100000000}; // 100ms
                nanosleep(&ts, NULL);
                continue;
            } else {
                perror("Failed to receive message from input queue");
                break;
            }
        }

        // Send to socket
        bytes_sent = send(sockfd, msg.msg_text, strlen(msg.msg_text), 0);
        
        if (bytes_sent == -1) {
            perror("Socket send error");
            running = 0;
            break;
        }

        printf("Received from input queue and sent to server: %s\n", msg.msg_text);
    }

    return NULL;
}

void signal_handler(int sig) {
    printf("\nReceived signal %d, shutting down...\n", sig);
    running = 0;
}

void cleanup(void) {
    printf("Cleaning up resources...\n");
    
    // Close socket
    if (sockfd != -1) {
        close(sockfd);
    }

    // Note: We don't automatically remove message queues as they might be used by other processes
    // To manually remove them, use: ipcrm -q <msgid>
    // Or use msgctl(msgid, IPC_RMID, NULL) if you want to remove them programmatically
    
    printf("Cleanup complete\n");
}
