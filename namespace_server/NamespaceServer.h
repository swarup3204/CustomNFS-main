#ifndef NAMESPACESERVER_H
#define NAMESPACESERVER_H

#include <string>
#include <vector>
#include <map>
#include <mutex>

// Structure to represent a file server.
struct FileServer
{
	std::string serverId;
	std::string ip;
	int port;
	int fileCount;
};

class NamespaceServer
{
public:
	// Constructor: accepts file paths for metadata.
	NamespaceServer(const std::string &dirFile, const std::string &fileFile,
					const std::string &userFile, const std::string &dirMapFile);

	// Runs the server on the given port.
	void run(int port);

private:
	// Filenames for metadata.
	std::string dirFilename;
	std::string fileFilename;
	std::string userFilename;
	std::string dirMapFilename;

	// In-memory metadata.
	std::vector<std::string> directories;
	std::map<std::string, std::string> fileMapping;
	std::map<std::string, std::string> users;
	std::map<std::string, std::string> dirMapping;

	// Hard-coded file servers.
	std::vector<FileServer> fileServers;

	// Mutex to protect metadata.
	std::mutex nsMutex;

	// Metadata load/save functions.
	void loadMetadata();
	void saveMetadata();
	void loadDirMapping();
	void saveDirMapping();

	// Authentication.
	bool authenticate(const std::string &username, const std::string &password);

	// Filesystem operations.
	std::string listDirectory(const std::string &path);
	std::string createFile(const std::string &path);
	std::string makeDirectory(const std::string &path);
	std::string deletePath(const std::string &path);

	// File server forwarding.
	std::string forwardToFileServer(const std::string &cmd, const std::string &serverId);
	std::string sendRequestToServer(const std::string &ip, int port, const std::string &request);

	// Request handling.
	std::string handleRequest(const std::string &request);
};

#endif // NAMESPACESERVER_H
