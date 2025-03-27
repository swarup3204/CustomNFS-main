#include "NamespaceServer.h"
#include "../common/util.h"
#include "../common/protocol.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>

NamespaceServer::NamespaceServer(const std::string &dirFile, const std::string &fileFile, const std::string &userFile)
	: dirFilename(dirFile), fileFilename(fileFile), userFilename(userFile)
{
	loadMetadata();
}

// once scope is defined you can use all variables in that scope as it is
void NamespaceServer::loadMetadata()
{
	std::lock_guard<std::mutex> lock(nsMutex);
	// Load directories
	std::ifstream dirFileStream(dirFilename);
	std::string line;
	directories.clear();
	if (dirFileStream.is_open())
	{
		while (std::getline(dirFileStream, line))
		{
			line = trim(line);
			if (!line.empty())
			{
				directories.push_back(line);
			}
		}
		dirFileStream.close();
	}
	// Load file mappings
	std::ifstream fileFileStream(fileFilename);
	fileMapping.clear();
	if (fileFileStream.is_open())
	{
		while (std::getline(fileFileStream, line))
		{
			line = trim(line);
			if (!line.empty())
			{
				size_t pos = line.find("=");
				if (pos != std::string::npos)
				{
					std::string filepath = trim(line.substr(0, pos));
					std::string serverId = trim(line.substr(pos + 1));
					fileMapping[filepath] = serverId;
				}
			}
		}
		fileFileStream.close();
	}
	// Load users
	std::ifstream userFileStream(userFilename);
	users.clear();
	if (userFileStream.is_open())
	{
		while (std::getline(userFileStream, line))
		{
			line = trim(line);
			if (!line.empty())
			{
				size_t pos = line.find(":");
				if (pos != std::string::npos)
				{
					std::string username = trim(line.substr(0, pos));
					std::string password = trim(line.substr(pos + 1));
					users[username] = password;
				}
			}
		}
		userFileStream.close();
	}
}

void NamespaceServer::saveMetadata()
{
	std::lock_guard<std::mutex> lock(nsMutex);
	// Save directories
	std::ofstream dirFileStream(dirFilename, std::ios::trunc);
	for (const auto &d : directories)
	{
		dirFileStream << d << "\n";
	}
	dirFileStream.close();
	// Save file mappings
	std::ofstream fileFileStream(fileFilename, std::ios::trunc);
	for (const auto &pair : fileMapping)
	{
		fileFileStream << pair.first << " = " << pair.second << "\n";
	}
	fileFileStream.close();
}

bool NamespaceServer::authenticate(const std::string &username, const std::string &password)
{
	auto it = users.find(username);
	return (it != users.end() && it->second == password);
}

std::string NamespaceServer::listDirectory(const std::string &path)
{
	std::lock_guard<std::mutex> lock(nsMutex);
	std::ostringstream oss;
	for (const auto &d : directories)
	{
		if (d.find(path) == 0)
		{ // simple prefix match
			oss << d << " ";
		}
	}
	return oss.str();
}

std::string NamespaceServer::createFile(const std::string &path)
{
	std::lock_guard<std::mutex> lock(nsMutex);
	if (fileMapping.find(path) != fileMapping.end())
	{
		return "ERR FileAlreadyExists";
	}
	// For simplicity, always choose "Server1".
	std::string serverId = "Server1";
	fileMapping[path] = serverId;
	saveMetadata();
	return "OK " + serverId;
}

std::string NamespaceServer::makeDirectory(const std::string &path)
{
	std::lock_guard<std::mutex> lock(nsMutex);
	for (const auto &d : directories)
	{
		if (d == path)
			return "ERR DirectoryAlreadyExists";
	}
	directories.push_back(path);
	saveMetadata();
	return "OK";
}

std::string NamespaceServer::deletePath(const std::string &path)
{
	std::lock_guard<std::mutex> lock(nsMutex);
	bool found = false;
	if (fileMapping.erase(path) > 0)
	{
		found = true;
	}
	auto it = std::find(directories.begin(), directories.end(), path);
	if (it != directories.end())
	{
		directories.erase(it);
		found = true;
	}
	if (!found)
		return "ERR NotFound";
	saveMetadata();
	return "OK";
}

std::string NamespaceServer::handleRequest(const std::string &request)
{
	std::istringstream iss(request);
	std::string command;
	iss >> command;
	if (command == "LOGIN")
	{
		std::string username, password;
		iss >> username >> password;
		if (authenticate(username, password))
		{
			return "OK";
		}
		else
		{
			return "ERR InvalidCredentials";
		}
	}
	else if (command == "LIST")
	{
		std::string path;
		iss >> path;
		return "DIR " + path + ": " + listDirectory(path);
	}
	else if (command == "CREATE_FILE")
	{
		std::string path;
		iss >> path;
		return createFile(path);
	}
	else if (command == "MKDIR")
	{
		std::string path;
		iss >> path;
		return makeDirectory(path);
	}
	else if (command == "DELETE")
	{
		std::string path;
		iss >> path;
		return deletePath(path);
	}
	else
	{
		return "ERR UnknownCommand";
	}
}

void NamespaceServer::run(int port)
{
	int sockfd, newsockfd;
	struct sockaddr_in serv_addr, cli_addr;
	socklen_t clilen;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
	{
		std::cerr << "Error opening socket\n";
		return;
	}
	memset((char *)&serv_addr, 0, sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(port);

	if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		std::cerr << "Error on binding\n";
		close(sockfd);
		return;
	}
	listen(sockfd, 5);
	std::cout << "Namespace Server running on port " << port << "\n";

	clilen = sizeof(cli_addr);
	while (true)
	{
		newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
		if (newsockfd < 0)
		{
			std::cerr << "Error on accept\n";
			continue;
		}
		std::string line;
		while (readLine(newsockfd, line) > 0)
		{
			std::cout << "Received: " << line << "\n";
			std::string response = handleRequest(line);
			sendLine(newsockfd, response);
		}
		close(newsockfd);
	}
	close(sockfd);
}
