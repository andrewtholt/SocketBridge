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
#include <signal.h> // For signal handling

#define BUFFER_SIZE 4096
#define PIPE_NET_TO_APP_NAME "/tmp/net_to_pipe"  // Data from network (socket) to application (written by forwarder, read by external tools)
#define PIPE_APP_TO_NET_NAME "/tmp/pipe_to_net" // Data from application (external tools) to network (written by external tools, read by forwarder)

// Reconnection settings
#define RECONNECT_DELAY_SECONDS 5
#define MAX_RECONNECT_ATTEMPTS 0 // 0 for infinite attempts

// Global flag to signal threads to stop
volatile int keep_running = 1;

typedef struct {
    int *socket_fd_ptr;       // Pointer to the socket_fd in main
    int *pipe_app_to_net_fd_ptr; // Pointer to pipe_read_fd (from app to net) in main
    int *pipe_net_to_app_fd_ptr; // Pointer to pipe_write_fd (from net to app) in main
    int verbose;
} thread_data_t;

void error_exit(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

void print_usage() {
    fprintf(stderr, "Usage: your_program_name [OPTIONS]\n\n");
    fprintf(stderr, "This program connects to a network socket, reads data from it and writes to a named pipe,\n");
    fprintf(stderr, "and simultaneously reads from another named pipe and writes to the same socket.\n");
    fprintf(stderr, "It attempts to automatically reconnect to both the network and named pipes if connections are lost.\n\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  --help        Display this help message and exit.\n");
    fprintf(stderr, "  -h <address>  Specify the address of the port to connect to (e.g., localhost, 127.0.0.1).\n");
    fprintf(stderr, "  -p <port>     Specify the port number to connect to.\n");
    fprintf(stderr, "  -v            Enable verbose output for debugging.\n");
}

// Signal handler for graceful shutdown (e.g., Ctrl+C)
void sigint_handler(int signum) {
    fprintf(stderr, "\nSIGINT (%d) received. Shutting down...\n",signum);
    keep_running = 0;
}

// Thread function to read from socket and write to named pipe (net -> app)
void *socket_to_pipe_thread(void *arg) {
    thread_data_t *data = (thread_data_t *)arg;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;

    if (data->verbose) {
        printf("[SocketToPipeThread] Starting...\n");
    }

    while (keep_running) {
        int current_socket_fd = *(data->socket_fd_ptr);
        int current_pipe_fd = *(data->pipe_net_to_app_fd_ptr);

        // Wait for both socket and pipe to be valid
        if (current_socket_fd == -1 || current_pipe_fd == -1) {
            usleep(100000); // 100 ms
            continue;
        }

        bytes_received = recv(current_socket_fd, buffer, sizeof(buffer), 0);

        if (bytes_received > 0) {
            if (data->verbose) {
                printf("[SocketToPipeThread] Received %zd bytes from socket. Writing to named pipe '%s'.\n", bytes_received, PIPE_NET_TO_APP_NAME);
            }
            if (write(current_pipe_fd, buffer, bytes_received) == -1) {
                if (errno == EPIPE) { // No process has the pipe open for reading
                    if (data->verbose) printf("[SocketToPipeThread] Named pipe '%s' has no reader (EPIPE). Signalling main to reopen pipe.\n", PIPE_NET_TO_APP_NAME);
                    *(data->pipe_net_to_app_fd_ptr) = -1; // Invalidate pipe FD to trigger reopen
                } else {
                    perror("[SocketToPipeThread] Error writing to named pipe");
                }
                // Even if pipe write fails, try to keep reading from socket for now
            }
        } else if (bytes_received == 0) {
            // Peer has performed an orderly shutdown
            if (data->verbose) {
                printf("[SocketToPipeThread] Socket closed by peer. Signalling main for reconnection.\n");
            }
            *(data->socket_fd_ptr) = -1; // Invalidate socket to trigger reconnection
            // Wait for main thread to re-establish
            while(*(data->socket_fd_ptr) == -1 && keep_running) {
                usleep(100000);
            }
        } else if (bytes_received == -1) {
            // An error occurred (e.g., connection reset by peer, socket broken)
            if (errno == EBADF || errno == ENOTSOCK) {
                if (data->verbose) printf("[SocketToPipeThread] Socket descriptor invalid, likely being reconnected.\n");
            } else {
                perror("[SocketToPipeThread] Error receiving from socket");
            }
            *(data->socket_fd_ptr) = -1; // Invalidate socket to trigger reconnection
            // Wait for main thread to re-establish
            while(*(data->socket_fd_ptr) == -1 && keep_running) {
                usleep(100000);
            }
        }
    }

    if (data->verbose) {
        printf("[SocketToPipeThread] Exiting.\n");
    }
    return NULL;
}

// Thread function to read from named pipe and write to socket (app -> net)
void *pipe_to_socket_thread(void *arg) {
    thread_data_t *data = (thread_data_t *)arg;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    int original_flags;

    if (data->verbose) {
        printf("[PipeToSocketThread] Starting...\n");
    }

    while (keep_running) {
        int current_socket_fd = *(data->socket_fd_ptr);
        int current_pipe_fd = *(data->pipe_app_to_net_fd_ptr);

        // Wait for both socket and pipe to be valid
        if (current_socket_fd == -1 || current_pipe_fd == -1) {
            usleep(100000); // 100 ms
            continue;
        }

        // Set pipe to non-blocking temporarily for this read
        original_flags = fcntl(current_pipe_fd, F_GETFL, 0);
        fcntl(current_pipe_fd, F_SETFL, original_flags | O_NONBLOCK);

        bytes_read = read(current_pipe_fd, buffer, sizeof(buffer));

        // Restore blocking mode (if it was blocking before)
        fcntl(current_pipe_fd, F_SETFL, original_flags);

        if (bytes_read > 0) {
            if (data->verbose) {
                printf("[PipeToSocketThread] Read %zd bytes from named pipe '%s'. Writing to socket.\n", bytes_read, PIPE_APP_TO_NET_NAME);
            }
            if (send(current_socket_fd, buffer, bytes_read, 0) == -1) {
                perror("[PipeToSocketThread] Error sending to socket");
                *(data->socket_fd_ptr) = -1; // Invalidate socket to trigger reconnection
                // Wait for main thread to re-establish
                while(*(data->socket_fd_ptr) == -1 && keep_running) {
                    usleep(100000);
                }
            }
        } else if (bytes_read == 0) {
            // EOF on pipe: The writer end of the FIFO was closed.
            if (data->verbose) {
                printf("[PipeToSocketThread] Named pipe '%s' writer closed (EOF). Signalling main to reopen pipe.\n", PIPE_APP_TO_NET_NAME);
            }
            *(data->pipe_app_to_net_fd_ptr) = -1; // Invalidate pipe FD to trigger reopen
            // Wait for main thread to re-establish
            while(*(data->pipe_app_to_net_fd_ptr) == -1 && keep_running) {
                usleep(100000);
            }
        } else if (bytes_read == -1) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) { // EAGAIN/EWOULDBLOCK for non-blocking read
                perror("[PipeToSocketThread] Error reading from named pipe");
                // Potentially a more serious pipe error, signal main to reopen
                *(data->pipe_app_to_net_fd_ptr) = -1;
                 while(*(data->pipe_app_to_net_fd_ptr) == -1 && keep_running) {
                    usleep(100000);
                }
            }
            usleep(100000); // Prevent busy-waiting if no data
        }
    }

    if (data->verbose) {
        printf("[PipeToSocketThread] Exiting.\n");
    }
    return NULL;
}

// Helper to open a FIFO, handling EEXIST and blocking until both sides are open
int open_fifo_robustly(const char *fifo_name, int flags, int verbose) {
    int fd = -1;
    if (mkfifo(fifo_name, 0666) == -1 && errno != EEXIST) {
        perror("mkfifo");
        return -1;
    } else if (verbose && errno == EEXIST) {
        printf("Named pipe '%s' already exists.\n", fifo_name);
    }

    int attempts = 0;
    const int max_attempts_open = 10; // Avoid infinite loop if something is truly wrong
    while (keep_running && fd == -1) {
        if (verbose) {
            printf("Opening '%s' with flags %d...\n", fifo_name, flags);
        }
        fd = open(fifo_name, flags);
        if (fd == -1) {
            if (errno == ENXIO) { // No readers/writers yet
                if (verbose) printf("Waiting for other end of pipe '%s' to open (ENXIO).\n", fifo_name);
                sleep(1); // Wait a second before retrying
                attempts++;
                if (max_attempts_open > 0 && attempts >= max_attempts_open) {
                    fprintf(stderr, "Max open attempts for pipe '%s' reached. Giving up.\n", fifo_name);
                    return -1;
                }
            } else {
                perror("open fifo_name");
                sleep(1); // Other error, wait and retry
                attempts++;
                 if (max_attempts_open > 0 && attempts >= max_attempts_open) {
                    fprintf(stderr, "Max open attempts for pipe '%s' reached. Giving up.\n", fifo_name);
                    return -1;
                }
            }
        } else {
            if (verbose) printf("Successfully opened '%s'. FD: %d\n", fifo_name, fd);
        }
    }
    return fd;
}


int main(int argc, char *argv[]) {
    int socket_fd = -1; // Initialize to -1 to indicate no connection
    int pipe_app_to_net_fd = -1; // From app (external) to network (read by forwarder)
    int pipe_net_to_app_fd = -1; // From network to app (written by forwarder)
    char *address = NULL;
    int port = -1;
    int verbose = 0;
    struct sockaddr_in serv_addr;
    pthread_t tid1, tid2;
    thread_data_t thread_data;
    int reconnect_socket_attempts = 0;

    // Register signal handler for graceful shutdown
    signal(SIGINT, sigint_handler);

    // Parse command line arguments (same as before)
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

    // Prepare thread data - pass pointers to main's FDs
    thread_data.socket_fd_ptr = &socket_fd;
    thread_data.pipe_app_to_net_fd_ptr = &pipe_app_to_net_fd;
    thread_data.pipe_net_to_app_fd_ptr = &pipe_net_to_app_fd;
    thread_data.verbose = verbose;

    // Create threads (they will continuously check the FD pointers)
    if (verbose) {
        printf("Creating communication threads...\n");
    }
    if (pthread_create(&tid1, NULL, socket_to_pipe_thread, (void *)&thread_data) != 0) {
        error_exit("Error creating socket_to_pipe_thread");
    }
    if (pthread_create(&tid2, NULL, pipe_to_socket_thread, (void *)&thread_data) != 0) {
        error_exit("Error creating pipe_to_socket_thread");
    }

    // Main loop for connection management (socket and pipes)
    while (keep_running) {
        // --- Manage Socket Connection ---
        if (socket_fd == -1) {
            if (reconnect_socket_attempts > 0) {
                if (verbose) {
                    printf("Socket connection lost. Attempting reconnect in %d seconds...\n", RECONNECT_DELAY_SECONDS);
                }
                sleep(RECONNECT_DELAY_SECONDS);
            }

            if (!keep_running) break;

            if (MAX_RECONNECT_ATTEMPTS > 0 && reconnect_socket_attempts >= MAX_RECONNECT_ATTEMPTS) {
                fprintf(stderr, "Maximum socket reconnect attempts (%d) reached. Exiting.\n", MAX_RECONNECT_ATTEMPTS);
                keep_running = 0;
                break;
            }

            if (verbose) {
                printf("Attempting to connect to %s:%d (Attempt %d)...\n", address, port, reconnect_socket_attempts + 1);
            }

            if (socket_fd != -1) { // Close if it was left open from a previous attempt
                close(socket_fd);
                socket_fd = -1;
            }

            socket_fd = socket(AF_INET, SOCK_STREAM, 0);
            if (socket_fd < 0) {
                perror("ERROR creating socket for reconnection");
                reconnect_socket_attempts++;
                continue;
            }

            memset(&serv_addr, 0, sizeof(serv_addr));
            serv_addr.sin_family = AF_INET;
            serv_addr.sin_port = htons(port);
            if (inet_pton(AF_INET, address, &serv_addr.sin_addr) <= 0) {
                close(socket_fd);
                socket_fd = -1;
                fprintf(stderr, "Invalid address/ Address not supported for reconnection.\n");
                reconnect_socket_attempts++;
                continue;
            }

            if (connect(socket_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
                perror("ERROR connecting");
                close(socket_fd);
                socket_fd = -1;
                reconnect_socket_attempts++;
                continue;
            }

            if (verbose) {
                printf("Successfully reconnected to %s:%d.\n", address, port);
            }
            reconnect_socket_attempts = 0;
        }

        // --- Manage Pipe Connections ---
        // Pipe: Network to Application (Forwarder writes, external client reads)
        if (pipe_net_to_app_fd == -1) {
            if (pipe_net_to_app_fd != -1) { close(pipe_net_to_app_fd); } // Ensure closed
            pipe_net_to_app_fd = open_fifo_robustly(PIPE_NET_TO_APP_NAME, O_WRONLY, verbose);
            if (pipe_net_to_app_fd == -1) {
                fprintf(stderr, "Failed to open FIFO for network to application (%s). Retrying...\n", PIPE_NET_TO_APP_NAME);
                usleep(500000); // Wait before next retry
                continue; // Loop again immediately to try both pipe and socket if needed
            }
        }

        // Pipe: Application to Network (External client writes, forwarder reads)
        if (pipe_app_to_net_fd == -1) {
            if (pipe_app_to_net_fd != -1) { close(pipe_app_to_net_fd); } // Ensure closed
            pipe_app_to_net_fd = open_fifo_robustly(PIPE_APP_TO_NET_NAME, O_RDONLY, verbose);
            if (pipe_app_to_net_fd == -1) {
                fprintf(stderr, "Failed to open FIFO for application to network (%s). Retrying...\n", PIPE_APP_TO_NET_NAME);
                usleep(500000); // Wait before next retry
                continue; // Loop again immediately to try both pipe and socket if needed
            }
        }

        // If both socket and pipes are valid, main thread sleeps briefly
        if (socket_fd != -1 && pipe_app_to_net_fd != -1 && pipe_net_to_app_fd != -1) {
            usleep(500000); // 500 ms to avoid busy-waiting
        } else {
             usleep(100000); // Shorter sleep if still waiting for connections
        }
    }

    // Signal threads to stop and wait for them
    keep_running = 0; // Ensure threads see the stop signal
    if (verbose) {
        printf("Main thread: Signalling threads to stop and waiting...\n");
    }
    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);

    // Cleanup
    if (verbose) {
        printf("Main thread: Cleaning up resources...\n");
    }
    if (socket_fd != -1) {
        close(socket_fd);
    }
    if (pipe_app_to_net_fd != -1) {
        close(pipe_app_to_net_fd);
    }
    if (pipe_net_to_app_fd != -1) {
        close(pipe_net_to_app_fd);
    }

    // Unlink named pipes
    unlink(PIPE_NET_TO_APP_NAME);
    unlink(PIPE_APP_TO_NET_NAME);

    if (verbose) {
        printf("Program finished.\n");
    }

    return 0;
}
