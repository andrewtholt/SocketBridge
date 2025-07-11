# Makefile for netpipe_forwarder

# Compiler
CC = gcc

# Compiler flags
# -Wall: Enable all standard warnings
# -Wextra: Enable extra warnings
# -pthread: Link against the POSIX threads library
# -g: Include debug information (optional, remove for release builds)
CFLAGS = -Wall -Wextra -pthread -g

# Output executable name
TARGET = netpipe_forwarder

# Source file
SRCS = netpipe_forwarder.c

# Object files (derived from source files)
OBJS = $(SRCS:.c=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
	# Optionally remove the named pipes if they were created during a run
	# and you want to ensure a clean state for the next build/run.
	# Be careful with this if another process is using them!
	# rm -f /tmp/net_to_pipe /tmp/pipe_to_net
