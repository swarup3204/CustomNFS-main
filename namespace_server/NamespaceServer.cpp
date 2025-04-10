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
#include <openssl/sha.h>
#include <iomanip>
#include <sstream>

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
		return "/";
	size_t pos = path.rfind('/');
	if (pos == 0)
		return "/";
	else if (pos != std::string::npos)
		return path.substr(0, pos);
	return "";
}

// Helper function to extract the basename from a full path.
// For example, for "/home/swarup/hello.txt", returns "hello.txt".
static std::string getBaseName(const std::string &path)
{
	size_t pos = path.find_last_of('/');
	if (pos == std::string::npos)
		return path;
	return path.substr(pos + 1);
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
	std::ofstream dirFileStream(dirFilename, std::ios::trunc);
	for (const auto &d : directories)
		dirFileStream << d << "\n";
	dirFileStream.close();
	std::ofstream fileFileStream(fileFilename, std::ios::trunc);
	for (const auto &pair : fileMapping)
		fileFileStream << pair.first << " = " << pair.second << "\n";
	fileFileStream.close();
}

void NamespaceServer::saveDirMapping()
{
	std::ofstream mapFile(dirMapFilename, std::ios::trunc);
	for (const auto &pair : dirMapping)
		mapFile << pair.first << " = " << pair.second << "\n";
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
		if (pair.first.find(path) == 0)
			oss << pair.first << "\n";
	}
	return oss.str();
}
// Computes the SHA256 hash of a given string and returns it as a hex string.
std::string computeSHA256(const std::string &data)
{
	unsigned char hash[SHA256_DIGEST_LENGTH];
	SHA256((const unsigned char *)data.c_str(), data.size(), hash);

	std::ostringstream oss;
	for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
	{
		// Convert each byte to a hex string.
		oss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
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
		return "ERR FileAlreadyExists";

	size_t pos = path.rfind('/');
	std::string dir = (pos == 0) ? "/" : (pos != std::string::npos ? path.substr(0, pos) : "/");

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
		assignedServer = it->second;
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
		dirMapping[dir] = assignedServer;
		saveDirMapping();
	}
	for (auto &fs : fileServers)
	{
		if (fs.serverId == assignedServer)
		{
			fs.fileCount++;
			break;
		}
	}

	// Compute a unique hash for the full file path.
	std::string hashedFileName = computeSHA256(path);
	fileMapping[path] = assignedServer;
	saveMetadata();

	// Send the create command along with the hashed file name.
	std::string fsResponse = forwardToFileServer("CREATE " + hashedFileName, assignedServer);
	if (fsResponse == "OK")
		return "OK " + assignedServer;
	else
	{
		std::lock_guard<std::mutex> lock(nsMutex);
		fileMapping.erase(path);
		for (auto &fs : fileServers)
		{
			if (fs.serverId == assignedServer && fs.fileCount > 0)
			{
				fs.fileCount--;
				break;
			}
		}
		saveMetadata();
		return fsResponse;
	}
}

// Creates a new directory.
// Returns an error if the directory already exists, if the path is invalid,
// or if the parent directory does not exist.
std::string NamespaceServer::makeDirectory(const std::string &path)
{
	if (!isValidPath(path))
		return "ERR InvalidPath";

	std::lock_guard<std::mutex> lock(nsMutex);
	for (const auto &d : directories)
	{
		if (d == path)
			return "ERR DirectoryAlreadyExists";
	}

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

	// Forward a "MKDIR" command to all file servers that might store files under this directory.
	// Since file servers store only file basenames, they don't store directories.
	// (This is handled solely in metadata.)
	return "OK";
}

// Deletes a file or directory.
// If a file is deleted, forward a "DELETE" command to the assigned file server (using basename).
// If a directory is deleted, recursively delete all files (by forwarding "DELETE" commands)
// for each file that has a path prefix matching the directory.
std::string NamespaceServer::deletePath(const std::string &path)
{
	if (!isValidPath(path))
		return "ERR InvalidPath";

	std::lock_guard<std::mutex> lock(nsMutex);
	bool found = false;

	// First, if the exact path exists as a file, delete it.
	if (fileMapping.find(path) != fileMapping.end())
	{
		std::string serverId = fileMapping[path];
		std::string base = getBaseName(path);
		std::string fsResponse = forwardToFileServer("DELETE " + base, serverId);
		if (fsResponse != "OK")
			return fsResponse;
		fileMapping.erase(path);
		found = true;
	}

	// Next, if the path is a directory (or a parent of files/directories), delete recursively.
	// Collect directories to delete: any directory that equals the given path or has it as a prefix
	std::vector<std::string> dirsToDelete;
	for (const auto &d : directories)
	{
		if (d == path || (d.size() > path.size() && d.substr(0, path.size()) == path && d[path.size()] == '/'))
			dirsToDelete.push_back(d);
	}

	// Collect files to delete: any file whose full path equals the given path
	// or starts with the given directory path followed by '/'
	std::vector<std::string> filesToDelete;
	for (const auto &pair : fileMapping)
	{
		if (pair.first == path || (pair.first.size() > path.size() &&
								   pair.first.substr(0, path.size()) == path && pair.first[path.size()] == '/'))
		{
			filesToDelete.push_back(pair.first);
		}
	}

	// For each file, instruct its file server to delete the file (using basename)
	for (const auto &f : filesToDelete)
	{
		std::string serverId = fileMapping[f];
		std::string base = getBaseName(f);
		forwardToFileServer("DELETE " + base, serverId);
		fileMapping.erase(f);
		found = true;
	}

	// Remove all directories in dirsToDelete from the directories vector and dirMapping.
	for (const auto &d : dirsToDelete)
	{
		auto it = std::find(directories.begin(), directories.end(), d);
		if (it != directories.end())
		{
			directories.erase(it);
			found = true;
		}
		dirMapping.erase(d);
	}

	if (!found)
		return "ERR NotFound";

	saveDirMapping();
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
		return "ERR SocketError";
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
		return authenticate(username, password) ? "OK" : "ERR InvalidCredentials";
	}
	else if (command == "LIST")
	{
		std::string path;
		iss >> path;
		if (!isValidPath(path))
			return "ERR InvalidPath";
		std::string listing = listDirectory(path);
		return (listing.rfind("ERR", 0) == 0) ? listing : ("DIR " + path + "\nFiles:\n" + listing);
	}
	else if (command == "CREATE_FILE")
	{
		std::string path;
		iss >> path;
		if (!isValidPath(path))
			return "ERR InvalidPath";
		std::string result = createFile(path);
		if (result.substr(0, 3) != "OK ")
			return result;
		std::string serverId = result.substr(3);
		std::string fsResponse = forwardToFileServer("CREATE " + path, serverId);
		if (fsResponse == "OK")
			return result;
		else
		{
			{
				std::lock_guard<std::mutex> lock(nsMutex);
				fileMapping.erase(path);
				for (auto &fs : fileServers)
				{
					if (fs.serverId == serverId && fs.fileCount > 0)
					{
						fs.fileCount--;
						break;
					}
				}
				saveMetadata();
			}
			return fsResponse;
		}
	}
	else if (command == "MKDIR")
	{
		std::string path;
		iss >> path;
		if (!isValidPath(path))
			return "ERR InvalidPath";
		std::string result = makeDirectory(path);
		if (result != "OK")
			return result;
		// Forward a "MKDIR" command to all file servers.
		// Since file servers do not store directory structures, this might be a no-op on the file servers.
		// However, if you wish to delete files based on directory deletion later, you could optionally send a command.
		return "OK";
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
		// Compute the unique hash for the file
		std::string hashedFileName = computeSHA256(path);
		return forwardToFileServer("READ " + hashedFileName + " " + std::to_string(offset) + " " + std::to_string(length), serverId);
	}
	else if (command == "WRITE")
	{
		std::string path;
		size_t offset;
		iss >> path >> offset;
		if (!isValidPath(path))
			return "ERR InvalidPath";
		std::string data;
		std::getline(iss >> std::ws, data);
		auto it = fileMapping.find(path);
		if (it == fileMapping.end())
			return "ERR FileNotFound";
		std::string serverId = it->second;
		// Use the computed hash for the file identifier
		std::string hashedFileName = computeSHA256(path);
		return forwardToFileServer("WRITE " + hashedFileName + " " + std::to_string(offset) + " " + data, serverId);
	}
	else
		return "ERR UnknownCommand";
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
