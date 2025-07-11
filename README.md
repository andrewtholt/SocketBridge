# Socket Bridge - How to Use

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

## Command Line Options

### Basic Syntax

```bash
./bin/socket_bridge -h <host> -p <port> [-v]
./bin/socket_bridge --help
```

### Required Arguments

- `-h, --host <host>` - Server IP address or hostname
- `-p, --port <port>` - Server port number (1-65535)

### Optional Arguments

- `-v, --verbose` - Enable verbose output for debugging
- `--help` - Show detailed help information and exit

### Getting Help

```bash
# Show usage information
./bin/socket_bridge --help

# Show brief usage (also displayed on error)
./bin/socket_bridge
```

## Running the Program

### Basic Examples

```bash
# Connect to local server on port 8080
./bin/socket_bridge -h 127.0.0.1 -p 8080

# Connect to remote server with verbose output
./bin/socket_bridge -h example.com -p 80 -v

# Using long option names
./bin/socket_bridge --host 192.168.1.100 --port 3000 --verbose

# Connect to SSH server (port 22)
./bin/socket_bridge -h myserver.com -p 22

# Connect to HTTP server
./bin/socket_bridge -h www.example.com -p 80
```

### Program Output

**Normal mode:**
```
Connected to server 127.0.0.1:8080
Input pipe: /tmp/socket_bridge_input
Output pipe: /tmp/socket_bridge_output
```

**Verbose mode (-v):**
```
Socket Bridge starting with verbose mode enabled
Target: 127.0.0.1:8080
Setting up named pipes...
Created input pipe: /tmp/socket_bridge_input
Created output pipe: /tmp/socket_bridge_output
Creating socket...
Connecting to 127.0.0.1:8080...
Connected to server 127.0.0.1:8080
Input pipe: /tmp/socket_bridge_input
Output pipe: /tmp/socket_bridge_output
Creating reader threads...
Socket reader thread started
Pipe reader thread started
Bridge is now active. Press Ctrl+C to stop.
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

# Send binary data
base64 image.jpg > /tmp/socket_bridge_input
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

# Process data as it arrives
while IFS= read -r line; do
    echo "Received: $line"
done < /tmp/socket_bridge_output
```

## Example Usage Scenarios

### 1. Interactive Chat Client

```bash
# Terminal 1: Start the bridge with verbose output
./bin/socket_bridge -h chat.example.com -p 6667 -v

# Terminal 2: Send messages
while true; do
    read -p "Message: " msg
    echo "$msg" > /tmp/socket_bridge_input
done

# Terminal 3: Read responses
tail -f /tmp/socket_bridge_output
```

### 2. Web Server Communication

```bash
# Start bridge to web server
./bin/socket_bridge -h www.example.com -p 80

# Send HTTP request
cat << EOF > /tmp/socket_bridge_input
GET / HTTP/1.1
Host: www.example.com
Connection: close

EOF

# Read HTTP response
cat /tmp/socket_bridge_output
```

### 3. Automated Script Communication

```bash
#!/bin/bash
# Start bridge in background
./bin/socket_bridge -h 127.0.0.1 -p 8080 -v &
BRIDGE_PID=$!

# Wait for connection
sleep 2

# Send authentication
echo "AUTH user:password" > /tmp/socket_bridge_input

# Send data and read response
echo "GET /status" > /tmp/socket_bridge_input
response=$(timeout 5 cat /tmp/socket_bridge_output)
echo "Server response: $response"

# Cleanup
kill $BRIDGE_PID
```

### 4. File Transfer

```bash
# Send a file (encode as base64 for safety)
base64 myfile.pdf > /tmp/socket_bridge_input

# Receive a file
cat /tmp/socket_bridge_output | base64 -d > received_file.pdf
```

### 5. JSON API Communication

```bash
# Send JSON request
echo '{"action": "get_user", "user_id": 123}' > /tmp/socket_bridge_input

# Read and process JSON response
response=$(timeout 10 cat /tmp/socket_bridge_output)
echo "$response" | jq '.result'
```

### 6. Database Connection

```bash
# Connect to database server
./bin/socket_bridge -h db.example.com -p 5432 -v

# Send database query (depends on protocol)
echo "SELECT * FROM users;" > /tmp/socket_bridge_input

# Read results
cat /tmp/socket_bridge_output
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
nohup ./bin/socket_bridge -h 127.0.0.1 -p 8080 -v > bridge.log 2>&1 &

# Check if it's running
ps aux | grep socket_bridge

# Stop background bridge
pkill -f socket_bridge
```

### Logging and Monitoring

```bash
# Run with logging (verbose mode recommended)
./bin/socket_bridge -h 127.0.0.1 -p 8080 -v 2>&1 | tee bridge.log

# Monitor both pipes simultaneously
tail -f /tmp/socket_bridge_input /tmp/socket_bridge_output

# Monitor with timestamps
tail -f /tmp/socket_bridge_output | while read line; do
    echo "$(date): $line"
done
```

### Shell Function for Easy Use

```bash
# Add to your .bashrc or .zshrc
socket_bridge() {
    if [ $# -lt 2 ]; then
        echo "Usage: socket_bridge <host> <port> [verbose]"
        return 1
    fi
    
    local host=$1
    local port=$2
    local verbose_flag=""
    
    if [ "$3" = "verbose" ] || [ "$3" = "v" ]; then
        verbose_flag="-v"
    fi
    
    ./bin/socket_bridge -h "$host" -p "$port" $verbose_flag
}

# Usage:
# socket_bridge 127.0.0.1 8080
# socket_bridge example.com 80 verbose
```

## Error Handling and Troubleshooting

### Common Error Messages

1. **Invalid arguments**
   ```
   Error: Both host (-h) and port (-p) are required.
   ```
   Solution: Provide both host and port parameters

2. **Invalid port number**
   ```
   Error: Invalid port number. Must be between 1 and 65535.
   ```
   Solution: Use a valid port number in the correct range

3. **Connection refused**
   ```
   Connection failed: Connection refused
   ```
   Solutions:
   - Check if the server is running
   - Verify the IP address and port
   - Check firewall settings
   - Try verbose mode for more details

4. **Permission denied on pipes**
   ```
   Failed to create input pipe: Permission denied
   ```
   Solutions:
   - Check write permissions to `/tmp/`
   - Try running with different user permissions
   - Ensure `/tmp/` directory exists

5. **Broken pipe errors**
   ```
   Failed to write to output pipe: Broken pipe
   ```
   Solutions:
   - Ensure a reader is connected to the output pipe
   - The pipe needs both a reader and writer

### Debug Mode

Use verbose mode to get detailed information about what the program is doing:

```bash
./bin/socket_bridge -h 127.0.0.1 -p 8080 -v
```

This will show:
- Connection attempts
- Pipe creation status
- Thread startup information
- Data transfer details
- Cleanup operations

### Checking Pipe Status

```bash
# List named pipes
ls -la /tmp/socket_bridge_*

# Check if pipes exist
test -p /tmp/socket_bridge_input && echo "Input pipe exists"
test -p /tmp/socket_bridge_output && echo "Output pipe exists"

# Check pipe permissions
stat /tmp/socket_bridge_input
stat /tmp/socket_bridge_output
```

### Testing Connection

```bash
# Test basic connectivity
telnet <host> <port>

# Test with netcat
nc <host> <port>

# Test DNS resolution
nslookup <hostname>
```

## Cleanup

### Manual Cleanup

```bash
# Remove named pipes
rm -f /tmp/socket_bridge_input /tmp/socket_bridge_output

# Or use make target
make clean-pipes

# Kill all socket_bridge processes
pkill -f socket_bridge
```

### Automatic Cleanup

The program automatically cleans up pipes when it exits normally (Ctrl+C or kill signal). However, if the program crashes or is killed forcefully, you may need to clean up manually.

## Security Considerations

- Named pipes in `/tmp/` are accessible to all users on the system
- Consider using a more secure location for pipes in production
- Be careful with sensitive data as it passes through the filesystem
- The program runs with the permissions of the user who started it
- Use verbose mode carefully as it may log sensitive data

## Performance Notes

- The program uses non-blocking I/O for optimal performance
- Large data transfers are handled efficiently
- Multiple clients can write to the input pipe simultaneously
- The output pipe supports multiple readers
- Verbose mode adds minimal overhead

## Stopping the Program

```bash
# Graceful shutdown (recommended)
kill -SIGINT <pid>
# or press Ctrl+C in the terminal

# Force stop (if graceful shutdown fails)
kill -SIGKILL <pid>
# or 
pkill -f socket_bridge
```

The program will automatically clean up resources and remove the named pipes when it exits gracefully.

## Integration Examples

### With systemd (Linux)

Create a service file `/etc/systemd/system/socket-bridge.service`:

```ini
[Unit]
Description=Socket Bridge Service
After=network.target

[Service]
Type=simple
User=nobody
ExecStart=/usr/local/bin/socket_bridge -h 127.0.0.1 -p 8080 -v
Restart=always
RestartSec=5

[Install]
WantedBy=multi-user.target
```

### With Docker

```dockerfile
FROM ubuntu:20.04
RUN apt-get update && apt-get install -y gcc make
COPY . /app
WORKDIR /app
RUN make
CMD ["./bin/socket_bridge", "-h", "host.docker.internal", "-p", "8080", "-v"]
```

### With Supervisor

```ini
[program:socket_bridge]
command=/usr/local/bin/socket_bridge -h 127.0.0.1 -p 8080 -v
autostart=true
autorestart=true
user=nobody
stdout_logfile=/var/log/socket_bridge.log
```
