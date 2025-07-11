#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include <time.h>
#include <fcntl.h>

#define MAX_MSG_SIZE 1024
#define INPUT_PIPE_NAME "/tmp/socket_bridge_input"
#define OUTPUT_PIPE_NAME "/tmp/socket_bridge_output"

// Global variables
int sockfd = -1;
int input_pipe_fd = -1;
int output_pipe_fd = -1;
volatile int running = 1;

// Function prototypes
void cleanup(void);
void signal_handler(int sig);
void* socket_reader_thread(void* arg);
void* pipe_reader_thread(void* arg);
int setup_socket(const char* server_ip, int port);
int setup_named_pipes(void);

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

    // Setup named pipes
    if (setup_named_pipes() != 0) {
        fprintf(stderr, "Failed to setup named pipes\n");
        exit(EXIT_FAILURE);
    }

    // Setup socket connection
    if (setup_socket(server_ip, port) != 0) {
        fprintf(stderr, "Failed to connect to server\n");
        exit(EXIT_FAILURE);
    }

    printf("Connected to server %s:%d\n", server_ip, port);
    printf("Input pipe: %s\n", INPUT_PIPE_NAME);
    printf("Output pipe: %s\n", OUTPUT_PIPE_NAME);

    // Create threads
    pthread_t socket_thread, pipe_thread;
    
    if (pthread_create(&socket_thread, NULL, socket_reader_thread, NULL) != 0) {
        perror("Failed to create socket reader thread");
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&pipe_thread, NULL, pipe_reader_thread, NULL) != 0) {
        perror("Failed to create pipe reader thread");
        exit(EXIT_FAILURE);
    }

    // Wait for threads to complete
    pthread_join(socket_thread, NULL);
    pthread_join(pipe_thread, NULL);

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

int setup_named_pipes(void) {
    // Create input pipe (for reading data to send to server)
    if (mkfifo(INPUT_PIPE_NAME, 0666) == -1) {
        if (errno != EEXIST) {
            perror("Failed to create input pipe");
            return -1;
        }
    }

    // Create output pipe (for writing data received from server)
    if (mkfifo(OUTPUT_PIPE_NAME, 0666) == -1) {
        if (errno != EEXIST) {
            perror("Failed to create output pipe");
            return -1;
        }
    }

    // Open input pipe for reading (non-blocking)
    input_pipe_fd = open(INPUT_PIPE_NAME, O_RDONLY | O_NONBLOCK);
    if (input_pipe_fd == -1) {
        perror("Failed to open input pipe");
        return -1;
    }

    // Open output pipe for writing
    output_pipe_fd = open(OUTPUT_PIPE_NAME, O_WRONLY);
    if (output_pipe_fd == -1) {
        perror("Failed to open output pipe");
        return -1;
    }

    return 0;
}

void* socket_reader_thread(void* arg) {
    (void)arg; // Suppress unused parameter warning
    char buffer[MAX_MSG_SIZE];
    ssize_t bytes_received, bytes_written;

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

        // Write to output pipe
        bytes_written = write(output_pipe_fd, buffer, bytes_received);
        
        if (bytes_written == -1) {
            perror("Failed to write to output pipe");
        } else {
            printf("Received from server and wrote to output pipe: %.*s\n", 
                   (int)bytes_received, buffer);
        }
    }

    return NULL;
}

void* pipe_reader_thread(void* arg) {
    (void)arg; // Suppress unused parameter warning
    char buffer[MAX_MSG_SIZE];
    ssize_t bytes_read, bytes_sent;

    while (running) {
        // Read from input pipe
        bytes_read = read(input_pipe_fd, buffer, sizeof(buffer) - 1);
        
        if (bytes_read == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // No data available, sleep briefly
                struct timespec ts = {0, 100000000}; // 100ms
                nanosleep(&ts, NULL);
                continue;
            } else {
                perror("Failed to read from input pipe");
                break;
            }
        } else if (bytes_read == 0) {
            // EOF - pipe was closed and reopened, or no writers
            struct timespec ts = {0, 100000000}; // 100ms
            nanosleep(&ts, NULL);
            continue;
        }

        // Null-terminate the data
        buffer[bytes_read] = '\0';

        // Send to socket
        bytes_sent = send(sockfd, buffer, bytes_read, 0);
        
        if (bytes_sent == -1) {
            perror("Socket send error");
            running = 0;
            break;
        }

        printf("Read from input pipe and sent to server: %.*s\n", 
               (int)bytes_read, buffer);
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

    // Close pipe file descriptors
    if (input_pipe_fd != -1) {
        close(input_pipe_fd);
    }
    
    if (output_pipe_fd != -1) {
        close(output_pipe_fd);
    }

    // Remove named pipes
    unlink(INPUT_PIPE_NAME);
    unlink(OUTPUT_PIPE_NAME);
    
    printf("Cleanup complete\n");
}
