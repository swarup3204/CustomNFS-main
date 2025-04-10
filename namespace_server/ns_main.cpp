#include "NamespaceServer.h"
#include <iostream>
#include <cstdlib>
#include <fstream>

void ensureFileExists(const std::string &filename, const std::string &defaultContent = "")
{
	std::ifstream infile(filename);
	if (!infile.good())
	{
		std::ofstream outfile(filename);
		outfile << defaultContent;
		outfile.close();
	}
}

int main(int argc, char *argv[])
{
	int port = 4000;
	if (argc > 1)
		port = std::atoi(argv[1]);

	std::string dirFile = "namespace_server/data/directories.txt";
	std::string fileFile = "namespace_server/data/files.txt";
	std::string userFile = "namespace_server/data/users.txt";
	std::string dirMapFile = "namespace_server/data/dirmapping.txt";

	ensureFileExists(dirFile, "/\n");
	ensureFileExists(fileFile);
	ensureFileExists(userFile);
	ensureFileExists(dirMapFile, "/ = Server1\n");

	NamespaceServer ns(dirFile, fileFile, userFile, dirMapFile);
	ns.run(port);
	return 0;
}
