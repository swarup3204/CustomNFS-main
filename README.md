# Stateless Network File System

## Overview

This project is a production-level simulation of a stateless network file system built in standard C++. It simulates a distributed file system on a single machine using multiple processes and ports:

- **Namespace Server**: Maintains directory structure and file-to-server mappings.
- **File Servers**: Store the actual file contents.
- **Clients**: Connect to the namespace server for metadata operations and to file servers for file I/O.

The system is designed to be stateless: every request contains the full file path and context, and the servers maintain no session state between requests. This simplifies recovery and scaling.

## Directory Structure
