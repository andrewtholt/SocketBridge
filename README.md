

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

Save the Makefile:
Save the provided Makefile content as Makefile (no file extension) in the same directory as netpipe\_forwarder.c.

Compile the program:
Open a terminal, navigate to the directory where you saved the files, and run:

```bash

make
```
This will compile the source code and create an executable named netpipe\_forwarder.

Clean (Optional):
To remove the generated object files and the executable, run:

```bash
make clean
Usage
Bash
make clean
```

```bash
./netpipe\_forwarder \[OPTIONS\]
Options:
\--help: Display the help message and exit.

\-h \<address\>: Specify the address of the port to connect to (e.g., localhost, 127.0.0.1, example.com). This option is required.

\-p \<port\>: Specify the port number to connect to (e.g., 80, 22, 12345). This option is required.

\-v: Enable verbose output for debugging.
```

## Run Instructions & Examples
Before running the netpipe\_forwarder, it's helpful to understand the flow:

Socket to Pipe: Data received from the network socket will be written to /tmp/net\_to\_pipe.

Pipe to Socket: Data written to /tmp/pipe\_to\_net will be sent to the network socket.

Example Setup (Using netcat for a simple server)
Let's assume you want to connect to a simple TCP server running on localhost (127.0.0.1) at port 12345\.

Start a simple TCP server (in Terminal 1):
You can use netcat (often nc) as a basic server:

```bash
nc \-l \-p 12345
```

This netcat instance will listen on port 12345\. Anything you type here will be sent to connected clients, and anything received from clients will be displayed here.

Run the netpipe\_forwarder (in Terminal 2):
Connect your forwarder to the netcat server:

```bash
./netpipe\_forwarder \-h 127.0.0.1 \-p 12345 \-v
```

You will see verbose output indicating the connection and thread creation.

Interact with the Named Pipes:

Sending data from a named pipe to the network (Terminal 3):
Write some text to the /tmp/pipe\_to\_net named pipe. This data will be sent to your netcat server.

```Bash
echo "Hello Netcat from Pipe\!" \> /tmp/pipe\_to\_net
```

You should immediately see "Hello Netcat from Pipe\!" appear in your Terminal 1 (netcat server).

Receiving data from the network in a named pipe (Terminal 4):
In Terminal 1 (netcat server), type some text and press Enter:

(In Netcat Terminal 1):
This is a message from the server.
Now, in a new Terminal 4, read from the /tmp/net\_to\_pipe named pipe. You will see the message from the server.

```bash
cat /tmp/net\_to\_pipe
```
cat will continue to display new data as it arrives. If you only want to read the current content and exit, you can use head \-n 1 /tmp/net\_to\_pipe or similar.
