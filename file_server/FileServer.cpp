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

FileServer::FileServer(const std::string &storageDir)
	: storageDirectory(storageDir)
{
	// Ensure the storage directory exists.
	mkdir(storageDirectory.c_str(), 0777);
}

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

std::string FileServer::writeFile(const std::string &path, size_t offset, const std::string &data)
{
	std::string fullPath = storageDirectory + path;
	// Open the file in read/write mode, create if it doesn't exist.
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
		data = trim(data);
		return writeFile(path, offset, data);
	}
	else if (command == "CREATE")
	{
		// Create an empty file.
		std::string path;
		iss >> path;
		std::string fullPath = storageDirectory + path;
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

void FileServer::processRequest(int clientSock)
{
	std::string line;
	while (readLine(clientSock, line) > 0)
	{
		std::cout << "FileServer received: " << line << "\n";
		std::string response = handleRequest(line);
		sendLine(clientSock, response);
	}
}

void FileServer::run(int port)
{
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
