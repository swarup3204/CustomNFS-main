#ifndef UTIL_H
#define UTIL_H

#include <string>

// Reads a message from the given socket.
// The message is expected to be prefixed by a 4-byte length field.
int readMessage(int sockfd, std::string &message);

// Sends a message to the given socket using a 4-byte length prefix.
int sendMessage(int sockfd, const std::string &message);

// Helper to trim whitespace from both ends of a string.
std::string trim(const std::string &str);

#endif // UTIL_H
