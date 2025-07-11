# SocketBridge# Socket Bridge - How to Use

## Overview

The Socket Bridge is a C program that creates a bidirectional bridge between a network socket and a pair of named pipes (FIFOs). It allows you to:

- Send data to a remote server through a named pipe
- Receive data from the server through another named pipe
- Use standard shell commands and scripts to interact with network services

## Building the Program

### Prerequisites

- GCC compiler
- POSIX-compliant system (Linux, macOS, etc.)
- Make build system

### Compilation

```bash
# Build the main program
make

# Or build just the socket bridge
make main

# Build with debug symbols
make debug

# Build optimized release version
make release
```

### Installation (Optional)

```bash
# Install to /usr/local/bin (requires sudo)
sudo make install

# Remove from system
sudo make uninstall
```

## Running the Program

### Basic Usage

```bash
./bin/socket_bridge <server_ip> <port>
```

### Examples

```bash
# Connect to a local server on port 8080
./bin/socket_bridge 127.0.0.1 8080

# Connect to a remote server
./bin/socket_bridge 192.168.1.100 3000

# Connect to a web server (though HTTP has specific protocol requirements)
./bin/socket_bridge www.example.com 80
```

### Program Output

When started successfully, the program will display:

```
Connected to server 127.0.0.1:8080
Input pipe: /tmp/socket_bridge_input
Output pipe: /tmp/socket_bridge_output
```

## Using the Named Pipes

### Named Pipe Locations

- **Input Pipe**: `/tmp/socket_bridge_input` - Write data here to send to server
- **Output Pipe**: `/tmp/socket_bridge_output` - Read data from here received from server

### Sending Data to Server

```bash
# Send a simple message
echo "Hello Server" > /tmp/socket_bridge_input

# Send a file's contents
cat myfile.txt > /tmp/socket_bridge_input

# Send multiple lines
cat << EOF > /tmp/socket_bridge_input
Line 1
Line 2
Line 3
EOF

# Send data interactively
cat > /tmp/socket_bridge_input
# Type your message and press Ctrl+D when done
```

### Receiving Data from Server

```bash
# Read all available data (will block until data arrives)
cat /tmp/socket_bridge_output

# Read data with timeout
timeout 5 cat /tmp/socket_bridge_output

# Continuously monitor for new data
tail -f /tmp/socket_bridge_output

# Read data in the background
cat /tmp/socket_bridge_output &
```

## Example Usage Scenarios

### 1. Interactive Chat Client

```bash
# Terminal 1: Start the bridge
./bin/socket_bridge chat.example.com 6667

# Terminal 2: Send messages
while true; do
    read -p "Message: " msg
    echo "$msg" > /tmp/socket_bridge_input
done

# Terminal 3: Read responses
tail -f /tmp/socket_bridge_output
```

### 2. Automated Script Communication

```bash
#!/bin/bash
# Start bridge in background
./bin/socket_bridge 127.0.0.1 8080 &
BRIDGE_PID=$!

# Wait for connection
sleep 2

# Send authentication
echo "AUTH user:password" > /tmp/socket_bridge_input

# Send data and read response
echo "GET /status" > /tmp/socket_bridge_input
timeout 5 cat /tmp/socket_bridge_output

# Cleanup
kill $BRIDGE_PID
```

### 3. File Transfer

```bash
# Send a file
base64 myfile.pdf > /tmp/socket_bridge_input

# Receive a file
cat /tmp/socket_bridge_output | base64 -d > received_file.pdf
```

### 4. JSON API Communication

```bash
# Send JSON request
echo '{"action": "get_user", "user_id": 123}' > /tmp/socket_bridge_input

# Read JSON response
response=$(timeout 10 cat /tmp/socket_bridge_output)
echo "$response" | jq '.'
```

## Advanced Usage

### Using with Other Programs

```bash
# Pipe output from another program
./my_data_generator | cat > /tmp/socket_bridge_input

# Process server responses
mkfifo /tmp/processed_output
cat /tmp/socket_bridge_output | ./my_processor > /tmp/processed_output &
```

### Background Operation

```bash
# Run bridge in background
nohup ./bin/socket_bridge 127.0.0.1 8080 > bridge.log 2>&1 &

# Check if it's running
ps aux | grep socket_bridge

# Stop background bridge
pkill -f socket_bridge
```

### Logging and Monitoring

```bash
# Run with logging
./bin/socket_bridge 127.0.0.1 8080 2>&1 | tee bridge.log

# Monitor both pipes simultaneously
tail -f /tmp/socket_bridge_input /tmp/socket_bridge_output
```

## Troubleshooting

### Common Issues

1. **Connection refused**
   ```
   Connection failed: Connection refused
   ```
   - Check if the server is running
   - Verify the IP address and port
   - Check firewall settings

2. **Permission denied on pipes**
   ```
   Failed to create input pipe: Permission denied
   ```
   - Check write permissions to `/tmp/`
   - Try running with different user permissions

3. **Broken pipe errors**
   ```
   Failed to write to output pipe: Broken pipe
   ```
   - Ensure a reader is connected to the output pipe
   - The pipe needs both a reader and writer

### Debug Mode

```bash
# Build and run in debug mode
make debug
./bin/socket_bridge 127.0.0.1 8080
```

### Checking Pipe Status

```bash
# List named pipes
ls -la /tmp/socket_bridge_*

# Check if pipes exist
test -p /tmp/socket_bridge_input && echo "Input pipe exists"
test -p /tmp/socket_bridge_output && echo "Output pipe exists"
```

## Cleanup

### Manual Cleanup

```bash
# Remove named pipes
rm -f /tmp/socket_bridge_input /tmp/socket_bridge_output

# Or use make target
make clean-pipes
```

### Automatic Cleanup

The program automatically cleans up pipes when it exits normally. However, if the program crashes or is killed forcefully, you may need to clean up manually.

## Security Considerations

- Named pipes in `/tmp/` are accessible to all users on the system
- Consider using a more secure location for pipes in production
- Be careful with sensitive data as it passes through the filesystem
- The program runs with the permissions of the user who started it

## Performance Notes

- The program uses non-blocking I/O for optimal performance
- Large data transfers are handled efficiently
- Multiple clients can write to the input pipe simultaneously
- The output pipe supports multiple readers

## Stopping the Program

```bash
# Graceful shutdown
kill -SIGINT <pid>
# or press Ctrl+C in the terminal

# Force stop
kill -SIGKILL <pid>
# or 
pkill -f socket_bridge
```

The program will automatically clean up resources and remove the named pipes when it exits.
