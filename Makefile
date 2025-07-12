# Makefile for netpipe_forwarder and netpipe_connector

# Compiler
CC = gcc

# Compiler flags
# -Wall: Enable all standard warnings
# -Wextra: Enable extra warnings
# -pthread: Link against the POSIX threads library
# -g: Include debug information (optional, remove for release builds)
CFLAGS = -Wall -Wextra -g
PTHREAD_FLAGS = -pthread

# Target executables
TARGET_FORWARDER = netpipe_forwarder
TARGET_CONNECTOR = netpipe_connector

# Source files
SRCS_FORWARDER = netpipe_forwarder.c
SRCS_CONNECTOR = netpipe_connector.c

# Object files
OBJS_FORWARDER = $(SRCS_FORWARDER:.c=.o)
OBJS_CONNECTOR = $(SRCS_CONNECTOR:.c=.o)

.PHONY: all clean

all: $(TARGET_FORWARDER) $(TARGET_CONNECTOR)

$(TARGET_FORWARDER): $(OBJS_FORWARDER)
	$(CC) $(CFLAGS) $(PTHREAD_FLAGS) $(OBJS_FORWARDER) -o $(TARGET_FORWARDER)

$(TARGET_CONNECTOR): $(OBJS_CONNECTOR)
	$(CC) $(CFLAGS) $(OBJS_CONNECTOR) -o $(TARGET_CONNECTOR)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS_FORWARDER) $(OBJS_CONNECTOR) $(TARGET_FORWARDER) $(TARGET_CONNECTOR)
	# Optionally remove the named pipes if they were created during a run
	# and you want to ensure a clean state for the next build/run.
	# Be careful with this if another process is using them!
	# rm -f /tmp/net_to_pipe /tmp/pipe_to_net
