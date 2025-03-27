#include "FileServer.h"
#include <iostream>
#include <cstdlib>

int main(int argc, char *argv[])
{
	int port = 4001;
	std::string storageDir = "file_server/storage";
	if (argc > 1)
	{
		port = std::atoi(argv[1]);
	}
	if (argc > 2)
	{
		storageDir = argv[2];
	}
	// Create a subdirectory for this file server instance.
	storageDir += "/server" + std::to_string(port);
	FileServer fs(storageDir);
	fs.run(port);
	return 0;
}
