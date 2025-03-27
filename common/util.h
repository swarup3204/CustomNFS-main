#include "util.h"
#include <unistd.h>
#include <sys/socket.h>
#include <cstring>
#include <iostream>

int readLine(int sockfd, std::string &line)
{
	line.clear();
	char ch;
	int n;
	while ((n = recv(sockfd, &ch, 1, 0)) > 0)
	{
		if (ch == '\n')
		{
			break;
		}
		line.push_back(ch);
	}
	return n;
}

int sendLine(int sockfd, const std::string &line)
{
	std::string msg = line + "\n";
	size_t totalSent = 0;
	while (totalSent < msg.size())
	{
		ssize_t sent = send(sockfd, msg.c_str() + totalSent, msg.size() - totalSent, 0);
		if (sent <= 0)
		{
			return sent;
		}
		totalSent += sent;
	}
	return totalSent;
}
