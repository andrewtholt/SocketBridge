#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <sys/select.h>

#define PIPE_NET_TO_APP_NAME "/tmp/net_to_pipe"  // Data from network to application (this connector reads from here)
#define PIPE_APP_TO_NET_NAME "/tmp/pipe_to_net" // Data from application to network (this connector writes here)
#define BUFFER_SIZE 4096

void print_usage(const char *prog_name) {
    fprintf(stderr, "Usage: %s\n", prog_name);
    fprintf(stderr, "This program connects to the named pipes created by netpipe_forwarder.\n");
    fprintf(stderr, "It forwards data from its standard input to the network via one pipe,\n");
    fprintf(stderr, "and forwards data from the network to its standard output via the other pipe.\n\n");
    fprintf(stderr, "Ensure netpipe_forwarder is running before starting this connector.\n");
}

int main(int argc, char *argv[]) {
    if (argc > 1 && strcmp(argv[1], "--help") == 0) {
        print_usage(argv[0]);
        exit(EXIT_SUCCESS);
    }

    int pipe_net_to_app_fd; // We read from this (data from network)
    int pipe_app_to_net_fd; // We write to this (data to network)
    char buffer[BUFFER_SIZE];
    fd_set read_fds;
    int max_fd;

    printf("Attempting to connect to pipes...\n");

    // Open the pipe for reading data from the network
    // This is the pipe the forwarder WRITES to. We READ from it.
    printf("Opening '%s' for reading...\n", PIPE_NET_TO_APP_NAME);
    pipe_net_to_app_fd = open(PIPE_NET_TO_APP_NAME, O_RDONLY);
    if (pipe_net_to_app_fd == -1) {
        perror("Failed to open pipe for reading (net_to_app)");
        fprintf(stderr, "Is the netpipe_forwarder running?\n");
        exit(EXIT_FAILURE);
    }
    printf("Pipe '%s' opened successfully (FD: %d).\n", PIPE_NET_TO_APP_NAME, pipe_net_to_app_fd);

    // Open the pipe for writing data to the network
    // This is the pipe the forwarder READS from. We WRITE to it.
    printf("Opening '%s' for writing...\n", PIPE_APP_TO_NET_NAME);
    pipe_app_to_net_fd = open(PIPE_APP_TO_NET_NAME, O_WRONLY);
    if (pipe_app_to_net_fd == -1) {
        perror("Failed to open pipe for writing (app_to_net)");
        close(pipe_net_to_app_fd);
        fprintf(stderr, "Is the netpipe_forwarder running?\n");
        exit(EXIT_FAILURE);
    }
    printf("Pipe '%s' opened successfully (FD: %d).\n", PIPE_APP_TO_NET_NAME, pipe_app_to_net_fd);

    printf("\nPipes connected. Forwarding data. Press Ctrl+D on stdin to exit.\n\n");

    // Determine the max file descriptor for select()
    max_fd = (STDIN_FILENO > pipe_net_to_app_fd ? STDIN_FILENO : pipe_net_to_app_fd) + 1;

    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);       // Watch stdin for user input
        FD_SET(pipe_net_to_app_fd, &read_fds); // Watch pipe for data from network

        int activity = select(max_fd, &read_fds, NULL, NULL, NULL);

        if (activity < 0 && errno != EINTR) {
            perror("select error");
            break;
        }

        // If stdin has data, read it and write to the pipe going to the network
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            ssize_t bytes_read = read(STDIN_FILENO, buffer, sizeof(buffer));
            if (bytes_read > 0) {
                // Check for the quit command
                if (bytes_read >= 5 && strncmp(buffer, "^quit", 5) == 0) {
                    printf("Quit command received. Shutting down.\n");
                    break; // Exit the loop
                }
                if (write(pipe_app_to_net_fd, buffer, bytes_read) == -1) {
                    perror("Error writing to pipe (app_to_net)");
                    break; // Exit on write error
                }
            } else if (bytes_read == 0) { // EOF from stdin (Ctrl+D)
                printf("Stdin closed. Shutting down write-end of the pipe.\n");
                break; // Exit the loop
            } else {
                perror("Error reading from stdin");
                break;
            }
        }

        // If the pipe from the network has data, read it and write to stdout
        if (FD_ISSET(pipe_net_to_app_fd, &read_fds)) {
            ssize_t bytes_read = read(pipe_net_to_app_fd, buffer, sizeof(buffer));
            if (bytes_read > 0) {
                if (write(STDOUT_FILENO, buffer, bytes_read) == -1) {
                    perror("Error writing to stdout");
                    break; // Exit on write error
                }
            } else if (bytes_read == 0) { // EOF from pipe
                printf("Network pipe closed by forwarder. Exiting.\n");
                break; // The other end (forwarder) closed the pipe
            } else {
                perror("Error reading from pipe (net_to_app)");
                break;
            }
        }
    }

    // Cleanup
    printf("Closing pipes...\n");
    close(pipe_net_to_app_fd);
    close(pipe_app_to_net_fd);

    printf("Connector finished.\n");

    return 0;
}