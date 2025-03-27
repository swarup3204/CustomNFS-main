#include "Client.h"
#include "../common/util.h"
#include "../common/protocol.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <sstream>

std::string Client::sendRequest(const std::string &host, int port, const std::string &request)
{
	int sockfd;
	struct sockaddr_in serv_addr;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
	{
		return "ERR SocketError";
	}
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	if (inet_pton(AF_INET, host.c_str(), &serv_addr.sin_addr) <= 0)
	{
		close(sockfd);
		return "ERR InvalidAddress";
	}
	if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		close(sockfd);
		return "ERR ConnectionFailed";
	}
	sendLine(sockfd, request);
	std::string response;
	readLine(sockfd, response);
	close(sockfd);
	return response;
}

Client::Client(const std::string &nsHost, int nsPort)
	: nsHost(nsHost), nsPort(nsPort)
{
}

bool Client::login(const std::string &username, const std::string &password)
{
	std::string req = "LOGIN " + username + " " + password;
	std::string resp = sendRequest(nsHost, nsPort, req);
	return resp == "OK";
}

std::string Client::list(const std::string &path)
{
	std::string req = "LIST " + path;
	return sendRequest(nsHost, nsPort, req);
}

std::string Client::createFile(const std::string &path)
{
	std::string req = "CREATE_FILE " + path;
	return sendRequest(nsHost, nsPort, req);
}

std::string Client::mkdir(const std::string &path)
{
	std::string req = "MKDIR " + path;
	return sendRequest(nsHost, nsPort, req);
}

std::string Client::deletePath(const std::string &path)
{
	std::string req = "DELETE " + path;
	return sendRequest(nsHost, nsPort, req);
}

std::string Client::readFile(const std::string &path, size_t offset, size_t length)
{
	// For simplicity, we assume the file is on Server1 at port 4001.
	std::string req = "READ " + path + " " + std::to_string(offset) + " " + std::to_string(length);
	return sendRequest(nsHost, nsPort, req);
}

std::string Client::writeFile(const std::string &path, size_t offset, const std::string &data)
{
	// For simplicity, we assume the file is on Server1 at port 4001.
	std::string req = "WRITE " + path + " " + std::to_string(offset) + " " + data;
	return sendRequest(nsHost, nsPort, req);
}
