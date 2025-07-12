

# Readme
## NetPipe Forwarder

This program acts as a bridge between a network socket and two named pipes (FIFOs). It simultaneously reads data from a connected network socket and writes it to one named pipe, while also reading data from another named pipe and sending it to the same network socket.

This can be useful for debugging network applications, piping network traffic to and from other command-line tools, or integrating network communication into shell scripts.

## Features

Connects to a specified remote host and port.

Reads data from the network socket and writes it to a named pipe (/tmp/net\_to\_pipe).

Reads data from another named pipe (/tmp/pipe\_to\_net) and sends it to the network socket.

Uses pthreads for concurrent read/write operations to prevent blocking.

Verbose mode for detailed debugging output.
## Prerequisites

A C compiler (e.g., GCC)

make utility (for building with the provided Makefile)

A Linux-like environment (uses POSIX named pipes and sockets)

Build Instructions
Save the source code:
Save the provided C code as netpipe\_forwarder.c in a directory.


## Compile the program:
Open a terminal, navigate to the directory where you saved the files, and run:

make

This will compile the source code and create an executable named netpipe\_forwarder.

Clean (Optional):
To remove the generated object files and the executable, run:

```bash
make clean
```
## Usage

./netpipe\_forwarder \[OPTIONS\]
Options:
\--help: Display the help message and exit.

\-h \<address\>: Specify the address of the port to connect to (e.g., localhost, 127.0.0.1, example.com). This option is required.

\-p \<port\>: Specify the port number to connect to (e.g., 80, 22, 12345). This option is required.

\-v: Enable verbose output for debugging.


### Run Instructions & Examples

A test program, netpipe_connector has been added.

Before running the netpipe_forwarder, it's helpful to understand the flow:

Socket to Pipe: Data received from the network socket will be written to /tmp/net_to_pipe.

Pipe to Socket: Data written to /tmp/pipe_to_net will be sent to the network socket.

Example Setup (Using netcat for a simple server)
Let's assume you want to connect to a simple TCP server running on localhost (127.0.0.1) at port 12345\.

Start a simple TCP server (in Terminal 1):
You can use netcat (often nc) as a basic server:

```bash
nc -l -p 12345
```

This netcat instance will listen on port 12345\. Anything you type here will be sent to connected clients, and anything received from clients will be displayed here.

Run the netpipe\_forwarder (in Terminal 2):
Connect your forwarder to the netcat server:

```bash
./netpipe\_forwarder -h 127.0.0.1 -p 12345 -v
```

You will see verbose output indicating the connection and thread creation.

Interact with the Named Pipes:

In, yet another terminal widow, do the following:
```bash
./netpipe_connector
```
If you now tipe into this window the text will be sent, via netpipe_forwarder, to the socket of the application.  Anything it returns will be displayed on the screen.

Sending data from a named pipe to the network (Terminal 3):
Write some text to the /tmp/pipe\_to\_net named pipe. This data will be sent to your netcat server.

To end the session type:

```bash
^quit
```



