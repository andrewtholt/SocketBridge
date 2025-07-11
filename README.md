### README

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

\`\`\`Bash

make  
\`\`\`

This will compile the source code and create an executable named netpipe\_forwarder.

Clean (Optional):  
To remove the generated object files and the executable, run:

Bash

make clean  
Usage  
Bash

./netpipe\_forwarder \[OPTIONS\]  
Options:  
\--help: Display the help message and exit.

\-h \<address\>: Specify the address of the port to connect to (e.g., localhost, 127.0.0.1, example.com). This option is required.

\-p \<port\>: Specify the port number to connect to (e.g., 80, 22, 12345). This option is required.

\-v: Enable verbose output for debugging.

Run Instructions & Examples  
Before running the netpipe\_forwarder, it's helpful to understand the flow:

Socket to Pipe: Data received from the network socket will be written to /tmp/net\_to\_pipe.

Pipe to Socket: Data written to /tmp/pipe\_to\_net will be sent to the network socket.

Example Setup (Using netcat for a simple server)  
Let's assume you want to connect to a simple TCP server running on localhost (127.0.0.1) at port 12345\.

Start a simple TCP server (in Terminal 1):  
You can use netcat (often nc) as a basic server:

Bash

nc \-l \-p 12345  
This netcat instance will listen on port 12345\. Anything you type here will be sent to connected clients, and anything received from clients will be displayed here.

Run the netpipe\_forwarder (in Terminal 2):  
Connect your forwarder to the netcat server:

Bash

./netpipe\_forwarder \-h 127.0.0.1 \-p 12345 \-v  
You will see verbose output indicating the connection and thread creation.

Interact with the Named Pipes:

Sending data from a named pipe to the network (Terminal 3):  
Write some text to the /tmp/pipe\_to\_net named pipe. This data will be sent to your netcat server.

Bash

echo "Hello Netcat from Pipe\!" \> /tmp/pipe\_to\_net  
You should immediately see "Hello Netcat from Pipe\!" appear in your Terminal 1 (netcat server).

Receiving data from the network in a named pipe (Terminal 4):  
In Terminal 1 (netcat server), type some text and press Enter:

(In Netcat Terminal 1):  
This is a message from the server.  
Now, in a new Terminal 4, read from the /tmp/net\_to\_pipe named pipe. You will see the message from the server.

Bash

cat /tmp/net\_to\_pipe  
cat will continue to display new data as it arrives. If you only want to read the current content and exit, you can use head \-n 1 /tmp/net\_to\_pipe or similar.

Important Notes:  
The named pipes (/tmp/net\_to\_pipe and /tmp/pipe\_to\_net) are created when the program starts. They will persist until the program exits and cleans them up, or if you manually remove them (rm /tmp/net\_to\_pipe /tmp/pipe\_to\_net).

Ensure that the named pipes are not being used by other processes when you start netpipe\_forwarder to avoid "Text file busy" or similar errors.

If the connection to the remote host is lost, the netpipe\_forwarder will typically exit.

produce a readme document in markdown format with usage and build instructions to be added to a file on git

NetPipe Forwarder  
The NetPipe Forwarder is a command-line utility designed to bridge communication between a network socket and two named pipes (FIFOs) on a Linux-like system. It simultaneously streams data from a connected network socket to one named pipe and reads data from another named pipe to send to the same socket.

This tool is invaluable for debugging network applications, integrating network communication into shell scripts, or piping network traffic to and from other command-line utilities.

Features  
Bidirectional Communication: Handles data flow in both directions between a network socket and named pipes.

Concurrent Operations: Utilizes POSIX threads (pthreads) to ensure non-blocking, simultaneous read/write operations.

Named Pipes (FIFOs): Leverages standard Linux named pipes for inter-process communication, making it easy to integrate with other command-line tools.

Configurable Connection: Connects to a specified remote host and port.

Verbose Mode: Offers detailed debugging output for troubleshooting.

Prerequisites  
Before you can build and run the NetPipe Forwarder, ensure you have the following installed on your system:

GCC (GNU Compiler Collection): A C compiler is required to compile the source code.

make Utility: Used to automate the build process with the provided Makefile.

Linux-like Environment: The program relies on POSIX named pipes and sockets, which are standard on Linux, macOS, and other Unix-like systems.

Build Instructions  
Follow these steps to compile the NetPipe Forwarder:

Save the Source Code:  
Save the C source code for the NetPipe Forwarder as netpipe\_forwarder.c in your desired project directory.

Save the Makefile:  
Create a file named Makefile (no extension) in the same directory as netpipe\_forwarder.c, and paste the following content into it:

Makefile

\# Makefile for netpipe\_forwarder

\# Compiler  
CC \= gcc

\# Compiler flags  
\# \-Wall: Enable all standard warnings  
\# \-Wextra: Enable extra warnings  
\# \-pthread: Link against the POSIX threads library  
\# \-g: Include debug information (optional, remove for release builds)  
CFLAGS \= \-Wall \-Wextra \-pthread \-g

\# Output executable name  
TARGET \= netpipe\_forwarder

\# Source file  
SRCS \= netpipe\_forwarder.c

\# Object files (derived from source files)  
OBJS \= $(SRCS:.c=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)  
        $(CC) $(CFLAGS) $(OBJS) \-o $(TARGET)

%.o: %.c  
        $(CC) $(CFLAGS) \-c $\< \-o $@

clean:  
        rm \-f $(OBJS) $(TARGET)  
        \# Optionally remove the named pipes if they were created during a run  
        \# and you want to ensure a clean state for the next build/run.  
        \# Be careful with this if another process is using them\!  
        \# rm \-f /tmp/net\_to\_pipe /tmp/pipe\_to\_net  
Compile:  
Open your terminal, navigate to the directory containing both files, and execute the make command:

Bash

make  
This command will compile netpipe\_forwarder.c and create an executable file named netpipe\_forwarder in the current directory.

Clean (Optional):  
To remove the generated object files (.o) and the executable, run:

Bash

make clean  
Usage  
Bash

./netpipe\_forwarder \[OPTIONS\]  
Command-Line Options:  
\--help: Displays the usage instructions and exits.

\-h \<address\>: Required. Specifies the target network address (e.g., localhost, 127.0.0.1, example.com).

\-p \<port\>: Required. Specifies the target network port number (e.g., 80, 22, 12345).

\-v: Enables verbose output, providing detailed debugging information about the program's operations.

Understanding the Data Flow:  
Network to Pipe: Data received from the connected network socket is written to the named pipe located at /tmp/net\_to\_pipe.

Pipe to Network: Data written to the named pipe located at /tmp/pipe\_to\_net is read by the forwarder and sent to the network socket.

Example Walkthrough (Using netcat for Testing)  
This example demonstrates how to use the NetPipe Forwarder with a simple netcat server.

Open Terminal 1 (Start a Listener):  
First, set up a basic TCP server using netcat that listens on port 12345\. This will act as our remote host.

Bash

nc \-l \-p 12345  
Any data typed into this terminal will be sent to connected clients, and any data received from clients will be displayed here.

Open Terminal 2 (Run NetPipe Forwarder):  
Now, run your netpipe\_forwarder program, connecting it to the netcat server you just started. Using the \-v flag is recommended for seeing the program's internal activity.

Bash

./netpipe\_forwarder \-h 127.0.0.1 \-p 12345 \-v  
You'll see output confirming the connection and the initiation of the communication threads.

Open Terminal 3 (Send Data from Pipe to Network):  
Write a message to the named pipe designated for sending data to the network (/tmp/pipe\_to\_net).

Bash

echo "Hello Netcat from Pipe\!" \> /tmp/pipe\_to\_net  
You should immediately see the message "Hello Netcat from Pipe\!" appear in Terminal 1 (your netcat server). This confirms data is flowing from your pipe, through the forwarder, to the network.

Open Terminal 1 (Send Data from Network to Pipe):  
Type a message into your netcat server terminal and press Enter.

(In Netcat Terminal 1):  
This is a message from the server.  
Open Terminal 4 (Receive Data from Network in Pipe):  
Now, read from the named pipe where data from the network is written (/tmp/net\_to\_pipe).

Bash

cat /tmp/net\_to\_pipe  
You should see "This is a message from the server." printed in this terminal. The cat command will continue to display any new data that arrives in the pipe. If you only want to read the current content and exit, you can use head \-n 1 /tmp/net\_to\_pipe.

Important Considerations:  
The named pipes (/tmp/net\_to\_pipe and /tmp/pipe\_to\_net) are created when netpipe\_forwarder starts. They will persist on your filesystem until the program exits (and cleans them up) or you manually remove them.

Ensure that no other processes are actively using the named pipes when you start the forwarder, as this could lead to errors like "Text file busy."

If the network connection to the remote host is terminated, the netpipe\_forwarder will typically detect this and exit gracefully.

This program provides a flexible way to integrate network streams with your shell environment. Feel free to experiment with different network services or command-line tools that can read from/write to pipes\!  
