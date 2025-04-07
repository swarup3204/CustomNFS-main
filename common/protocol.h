#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <string>
#include <sstream>
#include <vector>
#include <algorithm>

// Trim whitespace from both ends of a string.
// not needed yet
inline std::string trim(const std::string &s)
{
	size_t start = s.find_first_not_of(" \n\r\t");
	size_t end = s.find_last_not_of(" \n\r\t");
	if (start == std::string::npos || end == std::string::npos)
		return "";
	return s.substr(start, end - start + 1);
}

// Split a string by a delimiter (default is space).
inline std::vector<std::string> split(const std::string &s, char delimiter = ' ')
{
	std::vector<std::string> tokens;
	std::istringstream iss(s);
	std::string token;
	while (std::getline(iss, token, delimiter))
	{
		tokens.push_back(token);
	}
	return tokens;
}

#endif // PROTOCOL_H
