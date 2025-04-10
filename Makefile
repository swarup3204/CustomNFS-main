# Makefile for Custom Stateless Network File System

# Compiler and flags
CC = g++
CFLAGS = -std=c++17 -O2

# Directories
COMMON_DIR = common
NS_DIR = namespace_server
FS_DIR = file_server
CLIENT_DIR = client
EXTRAS_DIR = extras

# Targets (output executables)
NS_TARGET = NamespaceServer
FS_TARGET = FileServer
CLIENT_TARGET = Client
# CONCURRENCY_TARGET = ConcurrencyDemo

# Source files
NS_SRC = $(NS_DIR)/ns_main.cpp $(NS_DIR)/NamespaceServer.cpp $(COMMON_DIR)/util.cpp -lcrypto
FS_SRC = $(FS_DIR)/fs_main.cpp $(FS_DIR)/FileServer.cpp $(COMMON_DIR)/util.cpp
CLIENT_SRC = $(CLIENT_DIR)/client_main.cpp $(CLIENT_DIR)/Client.cpp $(COMMON_DIR)/util.cpp
# EXTRAS_SRC = $(EXTRAS_DIR)/concurrency_demo.cpp $(COMMON_DIR)/util.cpp

# Build all targets
all: $(NS_TARGET) $(FS_TARGET) $(CLIENT_TARGET) 

$(NS_TARGET): $(NS_SRC)
	$(CC) $(CFLAGS) -o $@ $^

$(FS_TARGET): $(FS_SRC)
	$(CC) $(CFLAGS) -o $@ $^

$(CLIENT_TARGET): $(CLIENT_SRC)
	$(CC) $(CFLAGS) -o $@ $^

$(CONCURRENCY_TARGET): $(EXTRAS_SRC)
	$(CC) $(CFLAGS) -pthread -o $@ $^

# Clean target to remove executables
clean:
	rm -f $(NS_TARGET) $(FS_TARGET) $(CLIENT_TARGET) $(CONCURRENCY_TARGET)
