#include "NamespaceServer.h"
#include "../common/util.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <climits>
#include <algorithm>

// Helper function to validate that a path is non-empty and starts with '/'
static bool isValidPath(const std::string &path)
{
	return !path.empty() && path[0] == '/';
}

// Helper function to extract the parent directory from a given path.
// For example, for "/home/swarup", it returns "/home". For paths directly under root, returns "/".
static std::string getParentDirectory(const std::string &path)
{
	if (path == "/")
	{
		return "/";
	}
	size_t pos = path.rfind('/');
	if (pos == 0)
	{
		return "/";
	}
	else if (pos != std::string::npos)
	{
		return path.substr(0, pos);
	}
	return "";
}

// Constructor: initializes file servers and loads metadata.
// Also ensures the root directory ("/") exists and is mapped.
NamespaceServer::NamespaceServer(const std::string &dirFile, const std::string &fileFile,
								 const std::string &userFile, const std::string &dirMapFile)
	: dirFilename(dirFile), fileFilename(fileFile), userFilename(userFile), dirMapFilename(dirMapFile)
{
	// Hard-code five file servers.
	fileServers.push_back({"Server1", "127.0.0.1", 4001, 0});
	fileServers.push_back({"Server2", "127.0.0.1", 4002, 0});
	fileServers.push_back({"Server3", "127.0.0.1", 4003, 0});
	fileServers.push_back({"Server4", "127.0.0.1", 4004, 0});
	fileServers.push_back({"Server5", "127.0.0.1", 4005, 0});

	loadMetadata();
	loadDirMapping();

	// Ensure the root directory "/" exists in the metadata.
	{
		std::lock_guard<std::mutex> lock(nsMutex);
		if (std::find(directories.begin(), directories.end(), "/") == directories.end())
		{
			directories.push_back("/");
			// saveMetadata() is called with the lock already held.
			saveMetadata();
		}
		if (dirMapping.find("/") == dirMapping.end())
		{
			dirMapping["/"] = "Server1";
			saveDirMapping();
		}
	}
}

// Loads directories, file mappings, and users from their respective files.
void NamespaceServer::loadMetadata()
{
	std::lock_guard<std::mutex> lock(nsMutex);
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
				std::cout << "Directory loaded: " << line << "\n";
			}
		}
		dirFileStream.close();
	}
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
					std::cout << "File mapping loaded: " << filepath << " -> " << serverId << "\n";
				}
			}
		}
		fileFileStream.close();
	}
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

// Note: These functions assume that the caller holds nsMutex.
void NamespaceServer::saveMetadata()
{
	// Caller must hold nsMutex.
	std::ofstream dirFileStream(dirFilename, std::ios::trunc);
	for (const auto &d : directories)
	{
		dirFileStream << d << "\n";
	}
	dirFileStream.close();
	std::ofstream fileFileStream(fileFilename, std::ios::trunc);
	for (const auto &pair : fileMapping)
	{
		fileFileStream << pair.first << " = " << pair.second << "\n";
	}
	fileFileStream.close();
}

void NamespaceServer::saveDirMapping()
{
	// Caller must hold nsMutex.
	std::ofstream mapFile(dirMapFilename, std::ios::trunc);
	for (const auto &pair : dirMapping)
	{
		mapFile << pair.first << " = " << pair.second << "\n";
	}
	mapFile.close();
}

void NamespaceServer::loadDirMapping()
{
	std::lock_guard<std::mutex> lock(nsMutex);
	std::ifstream mapFile(dirMapFilename);
	std::string line;
	dirMapping.clear();
	if (mapFile.is_open())
	{
		while (std::getline(mapFile, line))
		{
			line = trim(line);
			if (!line.empty())
			{
				size_t pos = line.find("=");
				if (pos != std::string::npos)
				{
					std::string dir = trim(line.substr(0, pos));
					std::string serverId = trim(line.substr(pos + 1));
					dirMapping[dir] = serverId;
				}
			}
		}
		mapFile.close();
	}
}

// Authenticates the user using the loaded credentials.
bool NamespaceServer::authenticate(const std::string &username, const std::string &password)
{
	auto it = users.find(username);
	return (it != users.end() && it->second == password);
}

// Lists the files under the given directory.
// Returns an error string if the directory does not exist or the path is invalid.
std::string NamespaceServer::listDirectory(const std::string &path)
{
	if (!isValidPath(path))
		return "ERR InvalidPath";

	std::lock_guard<std::mutex> lock(nsMutex);
	bool found = false;
	for (const auto &d : directories)
	{
		if (d == path)
		{
			found = true;
			break;
		}
	}
	if (!found)
		return "ERR DirectoryNotFound";

	std::ostringstream oss;
	for (const auto &pair : fileMapping)
	{
		// List files that have the directory as prefix.
		if (pair.first.find(path) == 0)
		{
			oss << pair.first << "\n";
		}
	}
	return oss.str();
}

// Creates a new file entry by assigning it to a file server.
// Returns an error if the file already exists or if the path is invalid.
std::string NamespaceServer::createFile(const std::string &path)
{
	if (!isValidPath(path))
		return "ERR InvalidPath";

	std::lock_guard<std::mutex> lock(nsMutex);
	if (fileMapping.find(path) != fileMapping.end())
	{
		return "ERR FileAlreadyExists";
	}
	// Extract the directory from the file path.
	size_t pos = path.rfind('/');
	std::string dir;
	if (pos == 0)
	{
		// File is in the root directory.
		dir = "/";
	}
	else if (pos != std::string::npos)
	{
		dir = path.substr(0, pos);
	}
	else
	{
		// Fallback to root.
		dir = "/";
	}

	// Check that the corresponding directory exists.
	bool parentFound = false;
	for (const auto &d : directories)
	{
		if (d == dir)
		{
			parentFound = true;
			break;
		}
	}
	if (!parentFound)
		return "ERR ParentDirectoryNotFound";

	std::string assignedServer;
	auto it = dirMapping.find(dir);
	if (it != dirMapping.end())
	{
		assignedServer = it->second;
	}
	else
	{
		int minCount = INT_MAX;
		for (auto &fs : fileServers)
		{
			if (fs.fileCount < minCount)
			{
				minCount = fs.fileCount;
				assignedServer = fs.serverId;
			}
		}
		// Record mapping for the directory.
		dirMapping[dir] = assignedServer;
		saveDirMapping();
	}
	// Increment the file count for the selected file server.
	for (auto &fs : fileServers)
	{
		if (fs.serverId == assignedServer)
		{
			fs.fileCount++;
			break;
		}
	}
	fileMapping[path] = assignedServer;
	saveMetadata();
	return "OK " + assignedServer;
}

// Creates a new directory.
// Returns an error if the directory already exists, if the path is invalid,
// or if the parent directory does not exist.
std::string NamespaceServer::makeDirectory(const std::string &path)
{
	if (!isValidPath(path))
		return "ERR InvalidPath";

	std::lock_guard<std::mutex> lock(nsMutex);

	// Ensure the directory does not already exist.
	for (const auto &d : directories)
	{
		if (d == path)
			return "ERR DirectoryAlreadyExists";
	}

	// Validate that the parent directory exists.
	std::string parent = getParentDirectory(path);
	bool parentFound = false;
	for (const auto &d : directories)
	{
		if (d == parent)
		{
			parentFound = true;
			break;
		}
	}
	if (!parentFound)
		return "ERR ParentDirectoryNotFound";

	directories.push_back(path);
	saveMetadata();
	return "OK";
}

// Deletes a file or directory.
// Returns an error if the path is not found or is invalid.
std::string NamespaceServer::deletePath(const std::string &path)
{
	if (!isValidPath(path))
		return "ERR InvalidPath";

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
		// Remove all files in the directory.
		for (auto it = fileMapping.begin(); it != fileMapping.end();)
		{
			if (it->first.find(path) == 0)
			{
				it = fileMapping.erase(it);
			}
			else
			{
				++it;
			}
		}
		dirMapping.erase(path);
		saveDirMapping();
	}
	if (!found)
		return "ERR NotFound";
	saveMetadata();
	return "OK";
}

// Forwards the given command to the appropriate file server.
std::string NamespaceServer::forwardToFileServer(const std::string &cmd, const std::string &serverId)
{
	for (const auto &fs : fileServers)
	{
		if (fs.serverId == serverId)
		{
			return sendRequestToServer(fs.ip, fs.port, cmd);
		}
	}
	return "ERR FileServerNotFound";
}

// Opens a socket connection to a file server, sends the request, and returns its response.
std::string NamespaceServer::sendRequestToServer(const std::string &ip, int port, const std::string &request)
{
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
	{
		return "ERR SocketError";
	}
	struct sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	if (inet_pton(AF_INET, ip.c_str(), &serv_addr.sin_addr) <= 0)
	{
		close(sockfd);
		return "ERR InvalidAddress";
	}
	if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		close(sockfd);
		return "ERR ConnectionFailed";
	}
	sendMessage(sockfd, request);
	std::string response;
	readMessage(sockfd, response);
	close(sockfd);
	return response;
}

// Parses and handles an incoming request, dispatching to the appropriate operation.
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
		if (!isValidPath(path))
			return "ERR InvalidPath";
		std::string listing = listDirectory(path);
		if (listing.rfind("ERR", 0) == 0)
		{
			return listing;
		}
		std::string str = "DIR " + path + "\nFiles:\n" + listing;
		return str;
	}
	else if (command == "CREATE_FILE")
	{
		std::string path;
		iss >> path;
		if (!isValidPath(path))
			return "ERR InvalidPath";

		// Create the file entry in metadata.
		std::string result = createFile(path);
		// If creation in metadata fails, return the error.
		if (result.substr(0, 3) != "OK ")
			return result;

		// Extract the assigned file server ID from the result.
		std::string serverId = result.substr(3);

		// Forward a "CREATE" command to the chosen file server.
		std::string fsResponse = forwardToFileServer("CREATE " + path, serverId);
		if (fsResponse == "OK")
		{
			return result; // Both metadata and file server creation succeeded.
		}
		else
		{
			// Roll back the metadata update:
			{
				std::lock_guard<std::mutex> lock(nsMutex);
				fileMapping.erase(path);
				// Optionally decrement the file count for the chosen file server.
				for (auto &fs : fileServers)
				{
					if (fs.serverId == serverId)
					{
						if (fs.fileCount > 0)
							fs.fileCount--;
						break;
					}
				}
				saveMetadata();
			}
			return fsResponse; // Propagate the file server error.
		}
	}
	else if (command == "MKDIR")
	{
		std::string path;
		iss >> path;
		if (!isValidPath(path))
			return "ERR InvalidPath";
		return makeDirectory(path);
	}
	else if (command == "DELETE")
	{
		std::string path;
		iss >> path;
		if (!isValidPath(path))
			return "ERR InvalidPath";
		return deletePath(path);
	}
	else if (command == "READ")
	{
		std::string path;
		size_t offset, length;
		iss >> path >> offset >> length;
		if (!isValidPath(path))
			return "ERR InvalidPath";
		auto it = fileMapping.find(path);
		if (it == fileMapping.end())
			return "ERR FileNotFound";
		std::string serverId = it->second;
		return forwardToFileServer("READ " + path + " " +
									   std::to_string(offset) + " " +
									   std::to_string(length),
								   serverId);
	}
	else if (command == "WRITE")
	{
		std::string path;
		size_t offset;
		iss >> path >> offset;
		if (!isValidPath(path))
			return "ERR InvalidPath";
		std::string data;
		std::getline(iss, data);
		auto it = fileMapping.find(path);
		if (it == fileMapping.end())
			return "ERR FileNotFound";
		std::string serverId = it->second;
		return forwardToFileServer("WRITE " + path + " " +
									   std::to_string(offset) + " " +
									   data,
								   serverId);
	}
	else
	{
		return "ERR UnknownCommand";
	}
}

// Runs the Namespace Server on the given port using the length-prefixed protocol.
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
		std::string message;
		while (readMessage(newsockfd, message) > 0)
		{
			std::cout << "Received: " << message << "\n";
			std::string response = handleRequest(message);
			sendMessage(newsockfd, response);
		}
		close(newsockfd);
	}
	close(sockfd);
}
