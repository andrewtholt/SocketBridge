# Makefile for Socket Message Queue Bridge
# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pedantic -O2
LDFLAGS = -lpthread
DEBUG_FLAGS = -g -DDEBUG -O0

# Directories
OBJDIR = obj
BINDIR = bin

# Source files (in current directory)
MAIN_SRC = socket_bridge.c
HELPER_SRCS = msg_send.c msg_receive.c test_server.c

# Object files
MAIN_OBJ = $(OBJDIR)/socket_bridge.o
HELPER_OBJS = $(OBJDIR)/msg_send.o $(OBJDIR)/msg_receive.o $(OBJDIR)/test_server.o

# Executable names
MAIN_TARGET = $(BINDIR)/socket_bridge
HELPER_TARGETS = $(BINDIR)/msg_send $(BINDIR)/msg_receive $(BINDIR)/test_server

# Default target - build main program and any helpers that have source files
all: directories $(MAIN_TARGET) helpers

# Build helpers only if source files exist
helpers:
	@if [ -f msg_send.c ]; then $(MAKE) $(BINDIR)/msg_send; fi
	@if [ -f msg_receive.c ]; then $(MAKE) $(BINDIR)/msg_receive; fi
	@if [ -f test_server.c ]; then $(MAKE) $(BINDIR)/test_server; fi

# Create directories
directories:
	@mkdir -p $(OBJDIR) $(BINDIR)

# Build just the main program if helper sources don't exist
main: directories $(MAIN_TARGET)

# Main program
$(MAIN_TARGET): $(MAIN_OBJ)
	$(CC) $(MAIN_OBJ) -o $@ $(LDFLAGS)
	@echo "Built main program: $@"

# Helper programs (only build if source files exist)
$(BINDIR)/msg_send: $(OBJDIR)/msg_send.o
	$(CC) $(OBJDIR)/msg_send.o -o $@
	@echo "Built helper program: $@"

$(BINDIR)/msg_receive: $(OBJDIR)/msg_receive.o
	$(CC) $(OBJDIR)/msg_receive.o -o $@
	@echo "Built helper program: $@"

$(BINDIR)/test_server: $(OBJDIR)/test_server.o
	$(CC) $(OBJDIR)/test_server.o -o $@ $(LDFLAGS)
	@echo "Built test server: $@"

# Object files - look for source files in current directory
$(OBJDIR)/socket_bridge.o: socket_bridge.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/msg_send.o: msg_send.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/msg_receive.o: msg_receive.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/test_server.o: test_server.c
	$(CC) $(CFLAGS) -c $< -o $@

# Debug build
debug: CFLAGS += $(DEBUG_FLAGS)
debug: clean all
	@echo "Built debug version"

# Release build (default optimization)
release: CFLAGS += -DNDEBUG
release: clean all
	@echo "Built release version"

# Install to system (requires sudo)
install: all
	@echo "Installing to /usr/local/bin..."
	sudo cp $(MAIN_TARGET) /usr/local/bin/
	sudo cp $(HELPER_TARGETS) /usr/local/bin/
	@echo "Installation complete"

# Uninstall from system
uninstall:
	@echo "Removing from /usr/local/bin..."
	sudo rm -f /usr/local/bin/socket_bridge
	sudo rm -f /usr/local/bin/msg_send
	sudo rm -f /usr/local/bin/msg_receive
	sudo rm -f /usr/local/bin/test_server
	@echo "Uninstallation complete"

# Clean up
clean:
	rm -rf $(OBJDIR) $(BINDIR)
	@echo "Cleaned up build files"

# Clean named pipes
clean-pipes:
	@echo "Cleaning up named pipes..."
	-rm -f /tmp/socket_bridge_input /tmp/socket_bridge_output
	@echo "Named pipes cleaned"

# Run tests
test: all
	@echo "Running basic tests..."
	@echo "Starting test server in background..."
	$(BINDIR)/test_server &
	@sleep 1
	@echo "Starting socket bridge in background..."
	$(BINDIR)/socket_bridge 127.0.0.1 8080 &
	@sleep 2
	@echo "Sending test message..."
	echo "Hello from named pipe" > /tmp/socket_bridge_input &
	@sleep 1
	@echo "Reading response..."
	timeout 2 cat /tmp/socket_bridge_output || echo "No response received"
	@echo "Stopping background processes..."
	@pkill -f socket_bridge || true
	@pkill -f test_server || true
	@echo "Test complete"

# Show help
help:
	@echo "Available targets:"
	@echo "  all          - Build all programs (default)"
	@echo "  debug        - Build with debug flags"
	@echo "  release      - Build optimized release version"
	@echo "  install      - Install to /usr/local/bin (requires sudo)"
	@echo "  uninstall    - Remove from /usr/local/bin (requires sudo)"
	@echo "  clean        - Remove build files"
	@echo "  clean-pipes  - Remove named pipes"
	@echo "  test         - Run basic functionality test"
	@echo "  help         - Show this help message"
	@echo ""
	@echo "Usage examples:"
	@echo "  make                    # Build all programs"
	@echo "  make debug             # Build debug version"
	@echo "  make clean-pipes           # Clean named pipes"
	@echo "  make install           # Install system-wide"
	@echo "  make test              # Run tests"

# Check for required dependencies
check-deps:
	@echo "Checking dependencies..."
	@which gcc > /dev/null || (echo "ERROR: gcc not found" && exit 1)
	@echo "✓ gcc found"
	@echo "✓ All dependencies satisfied"

# Show system info
info:
	@echo "System Information:"
	@echo "  OS: $(shell uname -s)"
	@echo "  Architecture: $(shell uname -m)"
	@echo "  Compiler: $(CC) $(shell $(CC) --version | head -1)"
	@echo "  Make: $(shell make --version | head -1)"
	@echo ""
	@echo "Build Configuration:"
	@echo "  CFLAGS: $(CFLAGS)"
	@echo "  LDFLAGS: $(LDFLAGS)"
	@echo "  Target: $(MAIN_TARGET)"

# Phony targets
.PHONY: all main helpers debug release install uninstall clean clean-pipes test help check-deps info directories
