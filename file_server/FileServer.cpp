#include "FileServer.h"
#include "../common/util.h"
#include "../common/protocol.h"

#include <iostream>
#include <sstream>
#include <fstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <sys/stat.h>

// FileServer constructor: accepts a storage directory prefix.
// (The actual storage directory will be updated in run() based on the port number.)
FileServer::FileServer(const std::string &storageDir)
	: storageDirectory(storageDir)
{
	// Initially, we just store the provided prefix.
	// The final directory will be set in run() using the port number.
}

// Reads a file from storageDirectory + path.
// The offset and length specify the part of the file to read.
// If the file does not exist, returns an error.
// Output format: "DATA <no of bytes read> <data>"
std::string FileServer::readFile(const std::string &path, size_t offset, size_t length)
{
	std::string fullPath = storageDirectory + path;
	std::ifstream in(fullPath, std::ios::binary);
	if (!in)
	{
		return "ERR FileNotFound";
	}
	in.seekg(offset, std::ios::beg);
	char *buffer = new char[length];
	in.read(buffer, length);
	size_t bytesRead = in.gcount();
	std::string data(buffer, bytesRead);
	delete[] buffer;
	return "DATA " + std::to_string(bytesRead) + " " + data;
}

// Writes data to a file at storageDirectory + path.
// The offset is used to write at a specific position.
// If the file does not exist, it is created.
// Output format: "OK <no of bytes written>".
std::string FileServer::writeFile(const std::string &path, size_t offset, const std::string &data)
{
	std::string fullPath = storageDirectory + path;
	// Open the file in read/write mode; if it doesn't exist, create it.
	std::fstream out;
	out.open(fullPath, std::ios::in | std::ios::out | std::ios::binary);
	if (!out.is_open())
	{
		out.clear();
		out.open(fullPath, std::ios::out | std::ios::binary);
		out.close();
		out.open(fullPath, std::ios::in | std::ios::out | std::ios::binary);
		if (!out.is_open())
		{
			return "ERR CannotOpenFile";
		}
	}
	out.seekp(offset, std::ios::beg);
	out.write(data.c_str(), data.size());
	out.flush();
	out.close();
	return "OK " + std::to_string(data.size());
}

// Processes a request string from the client and returns the appropriate response.
std::string FileServer::handleRequest(const std::string &request)
{
	// Expected formats:
	// READ <path> <offset> <length>
	// WRITE <path> <offset> <data>
	std::istringstream iss(request);
	std::string command;
	iss >> command;
	if (command == "READ")
	{
		std::string path;
		size_t offset, length;
		iss >> path >> offset >> length;
		return readFile(path, offset, length);
	}
	else if (command == "WRITE")
	{
		std::string path;
		size_t offset;
		iss >> path >> offset;
		std::string data;
		std::getline(iss, data);
		return writeFile(path, offset, data);
	}
	else if (command == "CREATE")
	{
		// Create an empty file.
		std::string path;
		iss >> path;

		char cwd[100];
		if (getcwd(cwd, sizeof(cwd)) == nullptr)
		{
			return "ERR CannotDetermineCWD";
		}
		// Construct the full path as: <cwd>/<storageDirectory>/<path>
		std::string fullPath = std::string(cwd) + "/" + storageDirectory + path;
		std::cout<<"Full path: "<<fullPath<<std::endl;
		// If the provided file path does not begin with a '/', add one.
		// if (!path.empty() && path[0] != '/')
		// 	fullPath += "/" + path;
		// else
		// 	fullPath += path;

		std::ofstream ofs(fullPath, std::ios::binary);
		if (ofs)
		{
			ofs.close();
			return "OK";
		}
		else
		{
			return "ERR CannotCreateFile";
		}
	}

	return "ERR UnknownCommand";
}

// Processes client requests on the given socket.
void FileServer::processRequest(int clientSock)
{
	std::string line;
	while (readMessage(clientSock, line) > 0)
	{
		std::cout << "FileServer received: " << line << "\n";
		std::string response = handleRequest(line);
		sendMessage(clientSock, response);
	}
}

// Runs the file server on the given port.
// Before binding the socket, the storage directory is updated to include a port-specific subdirectory.
void FileServer::run(int port)
{
	// Append a port-specific subdirectory.
	// storageDirectory = "/server" + std::to_string(port);
	// Ensure the directory exists.
	mkdir(storageDirectory.c_str(), 0777);

	int sockfd, newsockfd;
	struct sockaddr_in serv_addr, cli_addr;
	socklen_t clilen;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
	{
		std::cerr << "FileServer: Error opening socket\n";
		return;
	}
	memset((char *)&serv_addr, 0, sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(port);

	if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		std::cerr << "FileServer: Error on binding\n";
		close(sockfd);
		return;
	}
	listen(sockfd, 5);
	std::cout << "FileServer running on port " << port << "\n";
	clilen = sizeof(cli_addr);
	while (true)
	{
		newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
		if (newsockfd < 0)
		{
			std::cerr << "FileServer: Error on accept\n";
			continue;
		}
		processRequest(newsockfd);
		close(newsockfd);
	}
	close(sockfd);
}
