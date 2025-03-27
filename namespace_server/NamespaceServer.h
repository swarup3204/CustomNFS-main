#ifndef NAMESPACE_SERVER_H
#define NAMESPACE_SERVER_H

#include <string>
#include <unordered_map>
#include <mutex>
#include <vector>

class NamespaceServer
{
public:
	NamespaceServer(const std::string &dirFile, const std::string &fileFile, const std::string &userFile);
	void run(int port);

private:
	// Mapping of file paths to server IDs (e.g. "Server1")
	std::unordered_map<std::string, std::string> fileMapping;
	// List of directory paths
	std::vector<std::string> directories;
	// User credentials: username -> password
	std::unordered_map<std::string, std::string> users;

	std::string dirFilename;
	std::string fileFilename;
	std::string userFilename;

	std::mutex nsMutex; // Protects metadata

	void loadMetadata();
	void saveMetadata();
	std::string handleRequest(const std::string &request);
	bool authenticate(const std::string &username, const std::string &password);
	std::string listDirectory(const std::string &path);
	std::string createFile(const std::string &path);
	std::string makeDirectory(const std::string &path);
	std::string deletePath(const std::string &path);
};

#endif // NAMESPACE_SERVER_H
