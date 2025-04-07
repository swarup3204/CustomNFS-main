#include "util.h"
#include <unistd.h>
#include <sys/socket.h>
#include <cstring>
#include <arpa/inet.h>
#include <cctype>

// Helper: Trims whitespace from both ends of a string.
std::string trim(const std::string &str)
{
	size_t start = 0;
	while (start < str.size() && std::isspace(str[start]))
		start++;
	size_t end = str.size();
	while (end > start && std::isspace(str[end - 1]))
		end--;
	return str.substr(start, end - start);
}

// Reads a message that is prefixed with a 4-byte length field.
int readMessage(int sockfd, std::string &message)
{
	message.clear();
	uint32_t netLen;
	ssize_t n = recv(sockfd, &netLen, sizeof(netLen), 0);
	if (n <= 0)
	{
		return n;
	}
	uint32_t msgLen = ntohl(netLen);
	message.resize(msgLen);
	size_t totalRead = 0;
	while (totalRead < msgLen)
	{
		ssize_t bytesRead = recv(sockfd, &message[totalRead], msgLen - totalRead, 0);
		if (bytesRead <= 0)
		{
			return bytesRead;
		}
		totalRead += bytesRead;
	}
	return totalRead;
}

// Sends a message with a 4-byte length prefix.
int sendMessage(int sockfd, const std::string &message)
{
	uint32_t msgLen = message.size();
	uint32_t netLen = htonl(msgLen);
	// Send the length prefix.
	ssize_t sent = send(sockfd, &netLen, sizeof(netLen), 0);
	if (sent != sizeof(netLen))
		return -1;
	// Send the message body.
	size_t totalSent = 0;
	while (totalSent < msgLen)
	{
		ssize_t sentBytes = send(sockfd, message.c_str() + totalSent, msgLen - totalSent, 0);
		if (sentBytes <= 0)
			return sentBytes;
		totalSent += sentBytes;
	}
	return totalSent;
}
