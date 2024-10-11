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

#include <unistd.h>

// fix SOCK_NONBLOCK for OSX
#ifndef SOCK_NONBLOCK
#include <fcntl.h>
#define SOCK_NONBLOCK O_NONBLOCK
#endif

#define BACKLOG 5 // Allowed length of queue of waiting connections
#define LOGFILE "server.log"

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

void closeClient(int clientSocket, fd_set *openSockets, int *maxfds)
{

    printf("Client closed connection: %d\n", clientSocket);

    // If this client's socket is maxfds then the next lowest
    // one has to be determined. Socket fd's can be reused by the Kernel,
    // so there aren't any nice ways to do this.

    close(clientSocket);

    if (*maxfds == clientSocket)
    {
        for (auto const &p : clients)
        {
            *maxfds = std::max(*maxfds, p.second->sock);
        }
    }

    // And remove from the list of open sockets.

    FD_CLR(clientSocket, openSockets);
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
        logfile << std::ctime(&now) << " " << msg << std::endl;
        std::cout << std::ctime(&now) << " " << msg << std::endl;
        logfile.close();
    }
}

// Process command from client on the server

void clientCommand(int clientSocket, fd_set *openSockets, int *maxfds,
                   char *buffer)
{
    // parse the command. first check if the start is 0x01 and the end is 0x04, if not, ignore the command
    if (buffer[0] != 0x01 || buffer[strlen(buffer) - 1] != 0x04)
    {
        std::cout << "Invalid command from client" << std::endl;
        send(clientSocket, "Invalid command", 16, 0);
        return;
    }
    // Remove the start and end markers
    buffer[strlen(buffer) - 1] = '\0';

    // Process the command
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream stream(buffer + 1);

    while (std::getline(stream, token, ','))
        tokens.push_back(token);

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

        closeClient(clientSocket, openSockets, maxfds);
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
        // strip newline from tokens[2]
        std::string clientMsg = tokens[2];
        std::string msg = "Sending '" + clientMsg + "' to group number " + tokens[1];
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

int main(int argc, char *argv[])
{
    bool finished;
    int listenSock;       // Socket for connections to server
    int clientSock;       // Socket of connecting client
    fd_set openSockets;   // Current open sockets
    fd_set readSockets;   // Socket list for select()
    fd_set exceptSockets; // Exception socket list
    int maxfds;           // Passed to select() as max fd in set
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
    else
    // Add listen socket to socket set we are monitoring
    {
        FD_ZERO(&openSockets);
        FD_SET(listenSock, &openSockets);
        maxfds = listenSock;
    }

    finished = false;

    while (!finished)
    {
        // Get modifiable copy of readSockets
        readSockets = exceptSockets = openSockets;
        memset(buffer, 0, sizeof(buffer));

        // Look at sockets and see which ones have something to be read()
        int n = select(maxfds + 1, &readSockets, NULL, &exceptSockets, NULL);

        if (n < 0)
        {
            perror("select failed - closing down\n");
            finished = true;
        }
        else
        {
            // First, accept  any new connections to the server on the listening socket
            if (FD_ISSET(listenSock, &readSockets))
            {
                clientSock = accept(listenSock, (struct sockaddr *)&client,
                                    &clientLen);
                printf("Hello from Group_42\n");

                send(clientSock, "Helo, <Group_42\n", 21, 0);
                // Add new client to the list of open sockets
                FD_SET(clientSock, &openSockets);

                // And update the maximum file descriptor
                maxfds = std::max(maxfds, clientSock);

                // create a new client to store information.
                clients[clientSock] = new Client(clientSock);

                // Decrement the number of sockets waiting to be dealt with
                n--;

                printf("Client connected on server: %d\n", clientSock);
                printf("Maxfds is now: %d\n", maxfds);
            }
            // Now check for commands from clients
            std::list<Client *> disconnectedClients;
            while (n-- > 0)
            {
                for (auto const &pair : clients)
                {
                    Client *client = pair.second;

                    if (FD_ISSET(client->sock, &readSockets))
                    {
                        // recv() == 0 means client has closed connection
                        if (recv(client->sock, buffer, sizeof(buffer), MSG_DONTWAIT) == 0)
                        {
                            disconnectedClients.push_back(client);
                            closeClient(client->sock, &openSockets, &maxfds);
                        }
                        // We don't check for -1 (nothing received) because select()
                        // only triggers if there is something on the socket for us.
                        else
                        {
                            std::cout << buffer << std::endl;
                            clientCommand(client->sock, &openSockets, &maxfds, buffer);
                        }
                    }
                }
                // Remove client from the clients list
                for (auto const &c : disconnectedClients)
                    clients.erase(c->sock);
            }
        }
    }
}
