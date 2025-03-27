#ifndef FILE_SERVER_H
#define FILE_SERVER_H

#include <string>
#include <queue>
#include <mutex>

// Structure representing a file operation request.
struct FileOp
{
	std::string command; // e.g., READ or WRITE
	std::string path;
	size_t offset;
	size_t length;	  // For READ: number of bytes; for WRITE: length of data.
	std::string data; // Data for WRITE operations.
};

class FileServer
{
public:
	FileServer(const std::string &storageDir);
	void run(int port);

private:
	std::string storageDirectory;
	std::mutex fsMutex; // For potential concurrency (used in the threaded demo)

	// Processes a single client connection.
	void processRequest(int clientSock);
	// Parses and handles a request line.
	std::string handleRequest(const std::string &request);

	// Helper functions for file I/O.
	std::string readFile(const std::string &path, size_t offset, size_t length);
	std::string writeFile(const std::string &path, size_t offset, const std::string &data);
};

#endif // FILE_SERVER_H
