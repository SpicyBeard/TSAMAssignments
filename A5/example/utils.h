#ifndef UTILS_H
#define UTILS_H

#include <string.h>
#include <iostream>
#include <fstream>
#include <ctime>
#include <vector>
#include <fstream>
#include <sstream>

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <vector>
#include <map>
#include <cstring>
#include <utility>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netdb.h>
#include <fcntl.h>

using namespace std;

class Message
{
public:
    std::string message;
    std::string from;
    std::string to;
    std::string timestamp;

    Message(std::string message, std::string from, std::string to, std::string timestamp) : message(message), from(from), to(to), timestamp(timestamp) {}
    bool operator==(const Message &other) const
    {
        return message == other.message && from == other.from && to == other.to && timestamp == other.timestamp;
    }
};

class Client
{
public:
    int sock;         // socket of client connection
    std::string name; // Limit length of name of client's user
    std::string ip_address;
    int port;
    bool heloSent = false;
    std::vector<Client *> servers; // List of servers this client is connected to
    int misbehaveCounter = 0;
    time_t lastMessage = time(0);

    Client(int socket) : sock(socket) {}

    ~Client() {} // Virtual destructor defined for base class
};

int open_socket(int portno, string ip);

void sendMessage(Client client, const std::string &msg);

string receiveMessage(int sockfd);

pair<string, int> getSourceIpandPort(int sockfd);

bool valid_id(string id, map<int, Client *> &clients);

vector<vector<string>> checkMessageContentAndProcess(const string &input);

void logMessage(const std::string &msg, std::string filename, bool print);

bool connectedClient(int sock, map<int, Client *> &clients);

int connectToServer(int portno, const std::string &ip);

void sendKeepalive(Client client, int messages);

#endif // UTILS_H