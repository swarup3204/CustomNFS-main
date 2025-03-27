#ifndef CLIENT_H
#define CLIENT_H

#include <string>

class Client
{
public:
	Client(const std::string &nsHost, int nsPort);
	bool login(const std::string &username, const std::string &password);
	std::string list(const std::string &path);
	std::string createFile(const std::string &path);
	std::string mkdir(const std::string &path);
	std::string deletePath(const std::string &path);
	std::string readFile(const std::string &path, size_t offset, size_t length);
	std::string writeFile(const std::string &path, size_t offset, const std::string &data);

private:
	std::string nsHost;
	int nsPort;
	// Helper to send a request to a given host and port.
	std::string sendRequest(const std::string &host, int port, const std::string &request);
};

#endif // CLIENT_H
