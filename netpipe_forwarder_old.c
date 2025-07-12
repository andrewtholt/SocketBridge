

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <pthread.h>

#define BUFFER_SIZE 4096
#define PIPE_READ_NAME "/tmp/net_to_pipe"
#define PIPE_WRITE_NAME "/tmp/pipe_to_net"

typedef struct {
    int socket_fd;
    int pipe_read_fd;  // Pipe to read from for sending to socket
    int pipe_write_fd; // Pipe to write to from socket
    int verbose;
} thread_data_t;

void error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

void print_usage() {
    fprintf(stderr, "Usage: your_program_name [OPTIONS]\n\n");
    fprintf(stderr, "This program connects to a network socket, reads data from it and writes to a named pipe,\n");
    fprintf(stderr, "and simultaneously reads from another named pipe and writes to the same socket.\n\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  --help        Display this help message and exit.\n");
    fprintf(stderr, "  -h <address>  Specify the address of the port to connect to (e.g., localhost, 127.0.0.1).\n");
    fprintf(stderr, "  -p <port>     Specify the port number to connect to.\n");
    fprintf(stderr, "  -v            Enable verbose output for debugging.\n");
}

// Thread function to read from socket and write to named pipe
void *socket_to_pipe_thread(void *arg) {
    thread_data_t *data = (thread_data_t *)arg;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;

    if (data->verbose) {
        printf("[Thread 1] Starting socket_to_pipe_thread...\n");
    }

    while ((bytes_received = recv(data->socket_fd, buffer, sizeof(buffer), 0)) > 0) {
        if (data->verbose) {
            printf("[Thread 1] Received %zd bytes from socket. Writing to named pipe '%s'.\n", bytes_received, PIPE_READ_NAME);
            // Optionally print content, but be careful with non-printable characters
            // for (ssize_t i = 0; i < bytes_received; ++i) {
            //     printf("%02x ", (unsigned char)buffer[i]);
            // }
            // printf("\n");
        }
        if (write(data->pipe_write_fd, buffer, bytes_received) == -1) {
            perror("[Thread 1] Error writing to named pipe");
            break;
        }
    }

    if (bytes_received == 0) {
        if (data->verbose) {
            printf("[Thread 1] Socket closed by peer.\n");
        }
    } else if (bytes_received == -1) {
        perror("[Thread 1] Error receiving from socket");
    }

    if (data->verbose) {
        printf("[Thread 1] socket_to_pipe_thread exiting.\n");
    }
    return NULL;
}

// Thread function to read from named pipe and write to socket
void *pipe_to_socket_thread(void *arg) {
    thread_data_t *data = (thread_data_t *)arg;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    if (data->verbose) {
        printf("[Thread 2] Starting pipe_to_socket_thread...\n");
    }

    while ((bytes_read = read(data->pipe_read_fd, buffer, sizeof(buffer))) > 0) {
        if (data->verbose) {
            printf("[Thread 2] Read %zd bytes from named pipe '%s'. Writing to socket.\n", bytes_read, PIPE_WRITE_NAME);
            // Optionally print content
            // for (ssize_t i = 0; i < bytes_read; ++i) {
            //     printf("%02x ", (unsigned char)buffer[i]);
            // }
            // printf("\n");
        }
        if (send(data->socket_fd, buffer, bytes_read, 0) == -1) {
            perror("[Thread 2] Error sending to socket");
            break;
        }
    }

    if (bytes_read == 0) {
        if (data->verbose) {
            printf("[Thread 2] Named pipe '%s' closed.\n", PIPE_WRITE_NAME);
        }
    } else if (bytes_read == -1) {
        perror("[Thread 2] Error reading from named pipe");
    }

    if (data->verbose) {
        printf("[Thread 2] pipe_to_socket_thread exiting.\n");
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    int socket_fd = -1;
    int pipe_read_fd = -1;
    int pipe_write_fd = -1;
    char *address = NULL;
    int port = -1;
    int verbose = 0;
    struct sockaddr_in serv_addr;
    pthread_t tid1, tid2;
    thread_data_t thread_data;

    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            print_usage();
            exit(EXIT_SUCCESS);
        } else if (strcmp(argv[i], "-h") == 0) {
            if (i + 1 < argc) {
                address = argv[++i];
            } else {
                fprintf(stderr, "Error: -h requires an address argument.\n");
                print_usage();
                exit(EXIT_FAILURE);
            }
        } else if (strcmp(argv[i], "-p") == 0) {
            if (i + 1 < argc) {
                port = atoi(argv[++i]);
                if (port <= 0 || port > 65535) {
                    fprintf(stderr, "Error: Invalid port number.\n");
                    print_usage();
                    exit(EXIT_FAILURE);
                }
            } else {
                fprintf(stderr, "Error: -p requires a port number argument.\n");
                print_usage();
                exit(EXIT_FAILURE);
            }
        } else if (strcmp(argv[i], "-v") == 0) {
            verbose = 1;
        } else {
            fprintf(stderr, "Error: Unknown option '%s'\n", argv[i]);
            print_usage();
            exit(EXIT_FAILURE);
        }
    }

    if (address == NULL || port == -1) {
        fprintf(stderr, "Error: Both -h (address) and -p (port) are required.\n");
        print_usage();
        exit(EXIT_FAILURE);
    }

    if (verbose) {
        printf("Configuring: Address=%s, Port=%d, Verbose=%d\n", address, port, verbose);
    }

    // 1. Create named pipes (FIFOs)
    if (verbose) {
        printf("Creating named pipes...\n");
    }

    // Pipe for data from network to application
    if (mkfifo(PIPE_READ_NAME, 0666) == -1) {
        if (errno != EEXIST) { // If it exists, that's fine, just open it
            error("mkfifo PIPE_READ_NAME");
        } else {
            if (verbose) printf("Named pipe '%s' already exists.\n", PIPE_READ_NAME);
        }
    }

    // Pipe for data from application to network
    if (mkfifo(PIPE_WRITE_NAME, 0666) == -1) {
        if (errno != EEXIST) {
            error("mkfifo PIPE_WRITE_NAME");
        } else {
            if (verbose) printf("Named pipe '%s' already exists.\n", PIPE_WRITE_NAME);
        }
    }

    // 2. Create socket
    if (verbose) {
        printf("Creating socket...\n");
    }
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        error("ERROR opening socket");
    }

    // 3. Connect to server
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, address, &serv_addr.sin_addr) <= 0) {
        close(socket_fd);
        error("Invalid address/ Address not supported");
    }

    if (verbose) {
        printf("Connecting to %s:%d...\n", address, port);
    }
    if (connect(socket_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        close(socket_fd);
        error("ERROR connecting");
    }
    if (verbose) {
        printf("Successfully connected to %s:%d.\n", address, port);
    }

    // Open named pipes
    // Open PIPE_READ_NAME for writing (data from socket -> pipe)
    if (verbose) {
        printf("Opening named pipe '%s' for writing (socket to pipe)...\n", PIPE_READ_NAME);
    }
    pipe_write_fd = open(PIPE_READ_NAME, O_WRONLY);
    if (pipe_write_fd < 0) {
        close(socket_fd);
        error("ERROR opening PIPE_READ_NAME for writing");
    }
    if (verbose) {
        printf("Named pipe '%s' opened for writing.\n", PIPE_READ_NAME);
    }

    // Open PIPE_WRITE_NAME for reading (data from pipe -> socket)
    if (verbose) {
        printf("Opening named pipe '%s' for reading (pipe to socket)...\n", PIPE_WRITE_NAME);
    }
    pipe_read_fd = open(PIPE_WRITE_NAME, O_RDONLY);
    if (pipe_read_fd < 0) {
        close(pipe_write_fd);
        close(socket_fd);
        error("ERROR opening PIPE_WRITE_NAME for reading");
    }
    if (verbose) {
        printf("Named pipe '%s' opened for reading.\n", PIPE_WRITE_NAME);
    }

    // Prepare thread data
    thread_data.socket_fd = socket_fd;
    thread_data.pipe_read_fd = pipe_read_fd;
    thread_data.pipe_write_fd = pipe_write_fd;
    thread_data.verbose = verbose;

    // Create threads
    if (verbose) {
        printf("Creating threads...\n");
    }
    if (pthread_create(&tid1, NULL, socket_to_pipe_thread, (void *)&thread_data) != 0) {
        error("Error creating socket_to_pipe_thread");
    }
    if (pthread_create(&tid2, NULL, pipe_to_socket_thread, (void *)&thread_data) != 0) {
        error("Error creating pipe_to_socket_thread");
    }

    // Wait for threads to finish (they will typically run until socket/pipe closure)
    if (verbose) {
        printf("Waiting for threads to finish...\n");
    }
    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);

    // Cleanup
    if (verbose) {
        printf("Cleaning up resources...\n");
    }
    close(socket_fd);
    close(pipe_read_fd);
    close(pipe_write_fd);

    // Unlink named pipes - optional, but good for cleanup in long-running processes
    // comment out if you want to keep the pipes for external interaction
    unlink(PIPE_READ_NAME);
    unlink(PIPE_WRITE_NAME);

    if (verbose) {
        printf("Program finished.\n");
    }

    return 0;
}
