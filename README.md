# Stateless Network File System

## Overview

This project is a production-level simulation of a stateless network file system built in standard C++. It simulates a distributed file system on a single machine using multiple processes and ports:

- **Namespace Server**: Maintains directory structure and file-to-server mappings.
- **File Servers**: Store the actual file contents.
- **Clients**: Connect to the namespace server for metadata operations and to file servers for file I/O.

The system is designed to be stateless: every request contains the full file path and context, and the servers maintain no session state between requests. This simplifies recovery and scaling.

## Directory Structure

```plaintext
project/
├── README.md
├── design_document.tex
├── common/
│   ├── protocol.h
│   ├── util.h
│   └── util.cpp
├── namespace_server/
│   ├── NamespaceServer.h
│   ├── NamespaceServer.cpp
│   ├── ns_main.cpp
│   └── data/
│       ├── directories.txt
│       ├── files.txt
│       ├── users.txt
│       └── dirmapping.txt
├── file_server/
│   ├── FileServer.h
│   ├── FileServer.cpp
│   └── fs_main.cpp
├── client/
│   ├── Client.h
│   ├── Client.cpp
│   └── client_main.cpp
└── extras/
    └── concurrency_demo.cpp
```

## Running the Simulation

### 1. Start the Namespace Server

Open a terminal and run:

```bash
./NamespaceServer 4000
```

This starts the Namespace Server on port 4000. It loads metadata from `namespace_server/data/` (files such as `directories.txt`, `files.txt`, `users.txt`, and `dirmapping.txt`).

### 2. Start a File Server Instance

Open another terminal and run (for example, to start a File Server on port 4001):

```bash
./FileServer 4001
```

Files managed by this server will be stored in a subdirectory like `file_server/storage/server4001`.

> **Note**: The Namespace Server is configured with five File Servers. You can start additional File Server instances on ports 4002, 4003, 4004, and 4005 if needed.

### 3. Start a Client

Open a new terminal and run:

```bash
./Client
```

The Client connects to the Namespace Server at 127.0.0.1:4000 for all operations. The Namespace Server forwards READ and WRITE requests to the appropriate File Server based on file/directory mappings.

## Example Run

Below is an example interaction with the Client:

### Login

```plaintext
Enter username: alice
Enter password: password123
Login successful!
```

### List Directory

```plaintext
fs> ls /home/alice
DIR /home/alice
Files:
 /home/alice/notes.txt
 /home/alice/picture.jpg
```

### Create a New Directory

```plaintext
fs> mkdir /home/alice/newdir
OK Server1
```

This creates a new directory `/home/alice/newdir` and assigns it to the file server with the fewest files (e.g., Server1).

### Create a New File

```plaintext
fs> create /home/alice/newdir/hello.txt
OK Server1
```

The Namespace Server assigns the file to the same File Server as the directory.

### Write to the File

```plaintext
fs> write /home/alice/newdir/hello.txt 0 Hello World
OK 11
```

### Read from the File

```plaintext
fs> read /home/alice/newdir/hello.txt 0 11
DATA 11 Hello World
```

### Exit

```plaintext
fs> exit
```

## Metadata Files

Ensure the following files exist in the `namespace_server/data/` directory:

### directories.txt

Contains initial directory paths (one per line).
Example:

```
/home
/home/alice
/home/bob
```

### files.txt

Contains file-to-server mapping in the format:

```
/home/alice/notes.txt = Server1
/home/bob/report.pdf = Server2
```

### users.txt

Contains user credentials in the format:

```
alice:password123
bob:secure456
```

### dirmapping.txt

Contains directory-to-file server mapping in the format:

```
/home/alice = Server1
/home/bob = Server2
```

## Key Features

- **Stateless Architecture**: All requests are self-contained, improving fault tolerance
- **File-level Distribution**: Files are distributed across multiple servers
- **User Authentication**: Basic username/password authentication
- **Directory Hierarchy**: Supports standard file system operations
- **Concurrent Access**: Multiple clients can access the system simultaneously

## Building from Source

```bash
# Clone the repository
git clone https://github.com/username/stateless-nfs.git
cd stateless-nfs

# Build the project
make

# Alternatively, build individual components
make namespace_server
make file_server
make client
```

## System Requirements

- C++11 or higher
- POSIX-compliant operating system (Linux, macOS)
- Network connectivity between components
