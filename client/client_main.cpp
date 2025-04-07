#include "Client.h"
#include "../common/protocol.h" // For trim() function.
#include <iostream>
#include <sstream>
#include <string>

int main()
{
	Client client("127.0.0.1", 4000); // Connect to Namespace Server on port 4000
	std::string username, password;
	std::cout << "Enter username: ";
	std::cin >> username;
	std::cout << "Enter password: ";
	std::cin >> password;
	if (!client.login(username, password))
	{
		std::cout << "Login failed!\n";
		return 1;
	}
	std::cout << "Login successful!\n";

	std::cin.ignore(); // Clear newline character this is interesting
	std::string line;
	while (true)
	{
		std::cout << "fs> ";
		std::getline(std::cin, line);
		if (line == "exit")
			break;

		std::istringstream iss(line);
		std::string command;
		iss >> command;
		if (command == "ls")
		{
			std::string path;
			iss >> path;
			std::string resp = client.list(path);
			std::cout << resp << "\n";
		}
		else if (command == "mkdir")
		{
			std::string path;
			iss >> path;
			std::string resp = client.mkdir(path);
			std::cout << resp << "\n";
		}
		else if (command == "create")
		{
			std::string path;
			iss >> path;
			std::string resp = client.createFile(path);
			std::cout << resp << "\n";
		}
		else if (command == "rm")
		{
			std::string path;
			iss >> path;
			std::string resp = client.deletePath(path);
			std::cout << resp << "\n";
		}
		else if (command == "read")
		{
			std::string path;
			size_t offset, length;
			iss >> path >> offset >> length;
			// The Namespace Server forwards the read request to the appropriate file server.
			std::string resp = client.readFile(path, offset, length);
			std::cout << resp << "\n";
		}
		else if (command == "write")
		{
			std::string path;
			size_t offset;
			iss >> path >> offset;
			std::string data;
			std::getline(iss, data);
			data = trim(data);
			// The Namespace Server forwards the write request to the appropriate file server.
			std::string resp = client.writeFile(path, offset, data);
			std::cout << resp << "\n";
		}
		else
		{
			std::cout << "Unknown command\n";
		}
	}

	return 0;
}
