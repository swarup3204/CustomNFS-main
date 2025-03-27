#include "NamespaceServer.h"
#include <iostream>
#include <cstdlib>

int main(int argc, char *argv[])
{
	int port = 4000;
	if (argc > 1)
	{
		port = std::atoi(argv[1]);
	}
	// Files for metadata are stored in the "data" folder.
	std::string dirFile = "namespace_server/data/directories.txt";
	std::string fileFile = "namespace_server/data/files.txt";
	std::string userFile = "namespace_server/data/users.txt";

	NamespaceServer ns(dirFile, fileFile, userFile);
	ns.run(port);
	return 0;
}
