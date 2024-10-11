//
// Simple chat server for TSAM-409
//
// Command line: ./chat_server 4000
//
// Author: Jacky Mallett (jacky@ru.is)
//
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <algorithm>
#include <map>
#include <vector>
#include <list>
#include <fstream>
#include <ctime>
#include <iostream>
#include <sstream>
#include <thread>
#include <map>
#include <poll.h> // Add this for poll()

#include <unistd.h>

// fix SOCK_NONBLOCK for OSX
#ifndef SOCK_NONBLOCK
#include <fcntl.h>
#define SOCK_NONBLOCK O_NONBLOCK
#endif

#define BACKLOG 5 // Allowed length of queue of waiting connections
#define LOGFILE "server.log"
#define MAXSERVERS 8
#define MINSERVERS 3

// Simple class for handling connections from clients.
//
// Client(int socket) - socket to send/receive traffic from client.
class Client
{
public:
    int sock;         // socket of client connection
    std::string name; // Limit length of name of client's user

    Client(int socket) : sock(socket) {}

    ~Client() {} // Virtual destructor defined for base class
};

// Note: map is not necessarily the most efficient method to use here,
// especially for a server with large numbers of simulataneous connections,
// where performance is also expected to be an issue.
//
// Quite often a simple array can be used as a lookup table,
// (indexed on socket no.) sacrificing memory for speed.

std::map<int, Client *> clients; // Lookup table for per Client information

// Open socket for specified port.
//
// Returns -1 if unable to create the socket for any reason.

int open_socket(int portno)
{
    struct sockaddr_in sk_addr; // address settings for bind()
    int sock;                   // socket opened for this port
    int set = 1;                // for setsockopt

    // Create socket for connection. Set to be non-blocking, so recv will
    // return immediately if there isn't anything waiting to be read.
#ifdef __APPLE__
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Failed to open socket");
        return (-1);
    }
#else
    if ((sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) < 0)
    {
        perror("Failed to open socket");
        return (-1);
    }
#endif

    // Turn on SO_REUSEADDR to allow socket to be quickly reused after
    // program exit.

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &set, sizeof(set)) < 0)
    {
        perror("Failed to set SO_REUSEADDR:");
    }
    set = 1;
#ifdef __APPLE__
    if (setsockopt(sock, SOL_SOCKET, SOCK_NONBLOCK, &set, sizeof(set)) < 0)
    {
        perror("Failed to set SOCK_NOBBLOCK");
    }
#endif
    memset(&sk_addr, 0, sizeof(sk_addr));

    sk_addr.sin_family = AF_INET;
    sk_addr.sin_addr.s_addr = INADDR_ANY;
    sk_addr.sin_port = htons(portno);

    // Bind to socket to listen for connections from clients

    if (bind(sock, (struct sockaddr *)&sk_addr, sizeof(sk_addr)) < 0)
    {
        perror("Failed to bind to socket:");
        return (-1);
    }
    else
    {
        return (sock);
    }
}

// Close a client's connection, remove it from the client list, and
// tidy up select sockets afterwards.

// void closeClient(int clientSocket, fd_set *openSockets, int *maxfds)
// {

//     printf("Client closed connection: %d\n", clientSocket);

//     // If this client's socket is maxfds then the next lowest
//     // one has to be determined. Socket fd's can be reused by the Kernel,
//     // so there aren't any nice ways to do this.

//     close(clientSocket);

//     // And remove from the list of open sockets.

//     FD_CLR(clientSocket, openSockets);
// }

// Close a client's connection and remove it from pollfds
void closeClient(int clientSocket, std::vector<struct pollfd> &pollfds)
{
    printf("Client closed connection: %d\n", clientSocket);
    close(clientSocket);

    // Remove the socket from the pollfds vector
    auto it = std::remove_if(pollfds.begin(), pollfds.end(),
                             [clientSocket](struct pollfd &pfd)
                             {
                                 return pfd.fd == clientSocket;
                             });
    pollfds.erase(it, pollfds.end());
}

void logMessage(const std::string &msg)
{
    std::ofstream logfile;
    logfile.open(LOGFILE, std::ios::out | std::ios::app);
    if (!logfile.is_open())
    {
        std::cerr << "Failed to open log file" << std::endl;
        exit(1);
    }
    else
    {
        std::time_t now = std::time(0);
        char timeStr[100];
        std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", std::localtime(&now));

        logfile << timeStr << ": " << msg << std::endl;
        std::cout << timeStr << ": " << msg << std::endl;
        logfile.close();
    }
}

// Process command from client on the server

void clientCommand(int clientSocket, char *buffer)
{
    // parse the command. first check if the start is 0x01 and the end is 0x04, if not, ignore the command
    if (buffer[0] != 0x01 || buffer[strlen(buffer) - 1] != 0x04)
    {
        std::cout << "Invalid command from client" << std::endl;
        send(clientSocket, "Invalid command", 16, 0);
        return;
    }

    std::string input(buffer);

    // Optional: Allow commands without markers, but strip markers if they exist
    if (input[0] == 0x01)
    {
        input.erase(0, 1); // Remove starting 0x01 marker
    }
    if (input[input.length() - 1] == 0x04)
    {
        input.erase(input.length() - 1); // Remove ending 0x04 marker
    }

    // Trim any extra whitespaces, newline, etc.
    input.erase(0, input.find_first_not_of(" \n\r"));
    input.erase(input.find_last_not_of(" \n\r") + 1);

    std::cout << "Received command: " << input << std::endl;

    // Remove the start and end markers
    buffer[strlen(buffer) - 1] = '\0';

    // Process the command
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream stream(input);

    while (std::getline(stream, token, ','))
    {
        tokens.push_back(token);
    }

    if ((tokens[0].compare("CONNECT") == 0) && (tokens.size() == 2))
    {
        // TODO: figure out how to connect to other servers
        // std::cout << "Connecting to server: " << tokens[1] << std::endl;
        std::string msg = "Connecting to server: " + tokens[1];
        logMessage(msg);
        clients[clientSocket]->name = tokens[1];
    }
    else if (tokens[0].compare("LEAVE") == 0)
    {
        // Close the socket, and leave the socket handling
        // code to deal with tidying up clients etc. when
        // select() detects the OS has torn down the connection.

        closeClient(clientSocket, *new std::vector<struct pollfd>());
    }
    else if (tokens[0].compare("GETMSG") == 0 && tokens.size() == 2)
    {
        std::string msg = "Getting message from group number " + tokens[1];
        logMessage(msg);
        send(clientSocket, msg.c_str(), msg.length(), 0);
    }
    else if (tokens[0].compare("SENDMSG") == 0 && tokens.size() == 3)
    {
        // NOTE: if you dont know this group, forward to the groups you know and let them handle it
        std::string sanitizedToken2 = tokens[2];
        size_t pos = sanitizedToken2.find('\n');
        if (pos != std::string::npos)
        {
            sanitizedToken2.erase(pos, 1);
        }

        std::string msg = "Sending '" + sanitizedToken2 + "' to group number " + tokens[1];
        logMessage(msg);
        send(clientSocket, msg.c_str(), msg.length(), 0);
    }
    else if (tokens[0].compare("LISTSERVERS") == 0)
    {
        // TODO: for some reason, this is an unknown command
        // TODO: figure out how to list all servers we are connected to
        std::string msg = "Listing all servers we are connected to: ";
        ;
        for (auto server : clients)
        {
            msg += server.second->name + ", ";
        }
        logMessage(msg);
        send(clientSocket, msg.c_str(), msg.length(), 0);
    }
    else
    {
        std::string msg = "Unknown command from client: " + std::string(buffer);
        logMessage(msg);
        send(clientSocket, "Unknown command", 16, 0);
    }
}

// Remove fd_set and declare a vector of pollfd instead
std::vector<struct pollfd> pollfds;

// Helper function to remove client from poll list
void removeClientFromPoll(int clientSocket)
{
    auto it = std::remove_if(pollfds.begin(), pollfds.end(), [clientSocket](struct pollfd &pfd)
                             { return pfd.fd == clientSocket; });
    pollfds.erase(it, pollfds.end());
}

int main(int argc, char *argv[])
{
    bool finished;
    int listenSock; // Socket for connections to server
    int clientSock; // Socket of connecting client
    struct sockaddr_in client;
    socklen_t clientLen;
    char buffer[5000]; // buffer for reading from clients

    if (argc != 2)
    {
        printf("Usage: chat_server <ip port>\n");
        exit(0);
    }

    // Setup socket for server to listen to
    listenSock = open_socket(atoi(argv[1]));
    printf("Listening on port: %d\n", atoi(argv[1]));

    if (listen(listenSock, BACKLOG) < 0)
    {
        printf("Listen failed on port %s\n", argv[1]);
        exit(0);
    }

    // Add listening socket to the pollfds vector
    struct pollfd listenPollFD;
    listenPollFD.fd = listenSock;
    listenPollFD.events = POLLIN; // We are interested in when there's incoming connection
    pollfds.push_back(listenPollFD);

    finished = false;

    while (!finished)
    {
        memset(buffer, 0, sizeof(buffer));

        // Use poll() instead of select()
        int n = poll(pollfds.data(), pollfds.size(), -1); // Infinite timeout (-1)

        if (n < 0)
        {
            perror("poll failed - closing down\n");
            finished = true;
        }
        else
        {
            std::vector<size_t> clientsToRemove;
            // Loop through pollfds to check which file descriptor is ready
            for (size_t i = 0; i < pollfds.size(); i++)
            {
                if (pollfds[i].fd == listenSock && (pollfds[i].revents & POLLIN))
                {
                    if (pollfds.size() - 1 >= MAXSERVERS)
                    {
                        printf("Maximum number of clients reached. Refusing new connection.\n");
                        int tempSock = accept(listenSock, (struct sockaddr *)&client, &clientLen);
                        if (tempSock > 0)
                        {
                            send(tempSock, "Server full. Connection refused.\n", 35, 0);
                            close(tempSock);
                        }
                    }
                    else
                        // Check if it's the listening socket (new connection)
                        clientSock = accept(listenSock, (struct sockaddr *)&client, &clientLen);
                    if (clientSock > 0)
                    {
                        printf("Hello from Group_42\n");
                        send(clientSock, "Helo, <Group_42>\n", 21, 0);
                        printf("Client connected on server: %d\n", clientSock);

                        // Add new client to the pollfds vector
                        struct pollfd newClientPollFD;
                        newClientPollFD.fd = clientSock;
                        newClientPollFD.events = POLLIN; // We want to read from this socket
                        pollfds.push_back(newClientPollFD);

                        // Create a new client entry in the clients map
                        clients[clientSock] = new Client(clientSock);
                    }
                }
                // Check if an existing client has sent data
                else if (pollfds[i].revents & POLLIN)
                {
                    int clientSock = pollfds[i].fd;
                    int bytesReceived = recv(clientSock, buffer, sizeof(buffer), 0);

                    if (bytesReceived == 0)
                    {
                        // Client has disconnected
                        printf("Client disconnected: %d\n", clientSock);
                        closeClient(clientSock, pollfds);
                        clientsToRemove.push_back(i);
                        if (clients.find(clientSock) != clients.end())
                        {
                            delete clients[clientSock];
                            clients.erase(clientSock);
                        }
                        // removeClientFromPoll(clientSock); // Remove from poll list
                        // clients.erase(clientSock); // Remove from clients map
                    }
                    else if (bytesReceived > 0)
                    {
                        // Process the command from the client
                        clientCommand(clientSock, buffer);
                    }
                }
                // Check for errors or disconnection
                else if (pollfds[i].revents & (POLLERR | POLLHUP))
                {
                    int clientSock = pollfds[i].fd;
                    printf("Client disconnected due to error: %d\n", clientSock);
                    closeClient(clientSock, pollfds);
                    // removeClientFromPoll(clientSock); // Remove from poll list
                    clientsToRemove.push_back(i);
                    if (clients.find(clientSock) != clients.end())
                    {
                        delete clients[clientSock];
                        clients.erase(clientSock);
                    }
                }
            }
            for (size_t i : clientsToRemove)
            {
                pollfds.erase(pollfds.begin() + i);
            }
        }
    }
}
