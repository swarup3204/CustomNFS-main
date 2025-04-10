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
#include <errno.h>

// Helper function to extract the basename from a path.
static std::string getBaseName(const std::string &path)
{
	size_t pos = path.find_last_of('/');
	if (pos == std::string::npos)
		return path;
	return path.substr(pos + 1);
}

// FileServer constructor: accepts a storage directory prefix.
FileServer::FileServer(const std::string &storageDir)
	: storageDirectory(storageDir)
{
	// Ensure the base storage directory exists.
	mkdir(storageDirectory.c_str(), 0777);
}

// Reads a file from storageDirectory using only the file's basename.
std::string FileServer::readFile(const std::string &path, size_t offset, size_t length)
{
	std::string fileName = getBaseName(path);
	std::string fullPath = storageDirectory + "/" + fileName;
	std::ifstream in(fullPath, std::ios::binary);
	if (!in)
		return "ERR FileNotFound";
	in.seekg(offset, std::ios::beg);
	char *buffer = new char[length];
	in.read(buffer, length);
	size_t bytesRead = in.gcount();
	std::string data(buffer, bytesRead);
	delete[] buffer;
	return "DATA " + std::to_string(bytesRead) + " " + data;
}

// Writes data to a file in storageDirectory using only the file's basename.
std::string FileServer::writeFile(const std::string &path, size_t offset, const std::string &data)
{
	std::string fileName = getBaseName(path);
	std::string fullPath = storageDirectory + "/" + fileName;
	std::fstream out;
	out.open(fullPath, std::ios::in | std::ios::out | std::ios::binary);
	if (!out.is_open())
	{
		out.clear();
		out.open(fullPath, std::ios::out | std::ios::binary);
		out.close();
		out.open(fullPath, std::ios::in | std::ios::out | std::ios::binary);
		if (!out.is_open())
			return "ERR CannotOpenFile";
	}
	out.seekp(offset, std::ios::beg);
	out.write(data.c_str(), data.size());
	out.flush();
	out.close();
	return "OK " + std::to_string(data.size());
}

// Creates an empty file in storageDirectory using only the file's basename.
std::string FileServer::createFile(const std::string &path)
{
	std::string fileName = getBaseName(path);
	std::string fullPath = storageDirectory + "/" + fileName;
	std::ofstream ofs(fullPath, std::ios::binary);
	if (ofs)
	{
		ofs.close();
		return "OK";
	}
	else
		return "ERR CannotCreateFile";
}

// Deletes a file from storageDirectory using only the file's basename.
std::string FileServer::deleteFile(const std::string &path)
{
	std::string fileName = getBaseName(path);
	std::string fullPath = storageDirectory + "/" + fileName;
	if (remove(fullPath.c_str()) == 0)
		return "OK";
	else
		return "ERR CannotDeleteFile: " + std::string(strerror(errno));
}

// Handles incoming requests from the client.
std::string FileServer::handleRequest(const std::string &request)
{
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
		std::string path;
		iss >> path;
		return createFile(path);
	}
	else if (command == "DELETE")
	{
		std::string path;
		iss >> path;
		return deleteFile(path);
	}
	else if (command == "MKDIR")
	{
		// Although directories are not stored on file servers, we support this command
		// so that the nameserver can use it for rollback if needed.
		return "OK";
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
// Updates storageDirectory by appending a port-specific subdirectory, then creates that folder.
void FileServer::run(int port)
{
	// Append a port-specific subdirectory.
	storageDirectory = storageDirectory + "/server" + std::to_string(port);
	if (mkdir(storageDirectory.c_str(), 0777) != 0)
	{
		if (errno != EEXIST)
		{
			std::cerr << "FileServer: Failed to create storage directory: " << strerror(errno) << "\n";
			return;
		}
	}

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
