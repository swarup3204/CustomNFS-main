#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <queue>
#include <condition_variable>
#include "../common/util.h"
#include "../common/protocol.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>

std::mutex queueMutex;
std::condition_variable cv;
std::queue<int> clientQueue;

void workerThread()
{
	while (true)
	{
		int clientSock;
		{
			std::unique_lock<std::mutex> lock(queueMutex);
			cv.wait(lock, []
					{ return !clientQueue.empty(); });
			clientSock = clientQueue.front();
			clientQueue.pop();
		}
		std::string line;
		while (readLine(clientSock, line) > 0)
		{
			std::cout << "Worker thread processing: " << line << "\n";
			// For demonstration, simply echo back the received line.
			sendLine(clientSock, "ECHO: " + line);
		}
		close(clientSock);
	}
}

int main()
{
	int port = 5000;
	int sockfd, newsockfd;
	struct sockaddr_in serv_addr, cli_addr;
	socklen_t clilen;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
	{
		std::cerr << "ConcurrencyDemo: Error opening socket\n";
		return 1;
	}
	memset((char *)&serv_addr, 0, sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(port);

	if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		std::cerr << "ConcurrencyDemo: Error on binding\n";
		close(sockfd);
		return 1;
	}
	listen(sockfd, 5);
	std::cout << "Concurrency demo server running on port " << port << "\n";

	const int numThreads = 4;
	std::vector<std::thread> workers;
	for (int i = 0; i < numThreads; i++)
	{
		workers.emplace_back(workerThread);
	}

	clilen = sizeof(cli_addr);
	while (true)
	{
		newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
		if (newsockfd < 0)
		{
			std::cerr << "ConcurrencyDemo: Error on accept\n";
			continue;
		}
		{
			std::lock_guard<std::mutex> lock(queueMutex);
			clientQueue.push(newsockfd);
		}
		cv.notify_one();
	}

	for (auto &t : workers)
	{
		t.join();
	}

	close(sockfd);
	return 0;
}
