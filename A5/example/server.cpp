//
// Simple chat server for TSAM-409
//
// Command line: ./chat_server 4000
//
// Author: Lovisa & Dadi -- lovisa21@ru.is  -- dadir21@ru.is
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
#include "utils.h"

// fix SOCK_NONBLOCK for OSX
#ifndef SOCK_NONBLOCK
#include <fcntl.h>
#define SOCK_NONBLOCK O_NONBLOCK
#endif

#define BACKLOG 5 // Allowed length of queue of waiting connections
#define MAXSERVERS 8
#define MINSERVERS 3

#define MAXMISBEHAVIOUR 5

std::map<int, Client *> clients;    // Lookup table for per Client information
int main_client = -1;               // Main client socket
std::vector<Message> messageVector; // List of messages
std::map<string, int> messageMap;   // Map of messages

// Close a client's connection and remove it from pollfds

// Process command from client on the server

void clientCommand(int clientSocket, char *buffer)
{
    if (buffer == NULL)
    {
        std::string msg = "Invalid command from " + clients[clientSocket]->name + " at " + clients[clientSocket]->ip_address + ":" + std::to_string(clients[clientSocket]->port);
        logMessage(msg, "");
        clients[clientSocket]->misbehaveCounter++;
        return;
    }
    std::vector<std::string> tokens = checkMessageContentAndProcess(buffer);

    if (tokens[0].compare("Rattatoskur") == 0)
    {
        std::string msg = "Main Client connected to Server";
        logMessage(msg, "");
        send(clientSocket, msg.c_str(), msg.length(), 0);
        main_client = clientSocket;
    }
    else if (tokens[0].compare("HELO") == 0 && tokens.size() == 2)
    {
        logMessage("Received HELO from " + tokens[1] + " at " + clients[clientSocket]->ip_address + ":" + std::to_string(clients[clientSocket]->port), "");

        if (valid_id(tokens[1], clients))
        {
            if (clients[clientSocket]->name == tokens[1])
            {
                return;
            }
            clients[clientSocket]->name = tokens[1];
            std::string response = "SERVERS,A5_42,130.208.246.249,4042;";
            for (auto server : clients)
            {
                if (server.second->name != clients[clientSocket]->name)
                {
                    response += server.second->name + "," + server.second->ip_address + "," + std::to_string(server.second->port) + ";";
                }
            }
            if (!clients[clientSocket]->heloSent)
            {
                clients[clientSocket]->heloSent = true;
                //    sendMessage(clients[clientSocket], "HELO,A5_42");
            }
            std::string loggedMessage = "Sending server list to " + clients[clientSocket]->name + " at " + clients[clientSocket]->ip_address + ":" + std::to_string(clients[clientSocket]->port);
            logMessage("Sending connected server list to " + clients[clientSocket]->name + " at " + clients[clientSocket]->ip_address + ":" + std::to_string(clients[clientSocket]->port), "");
            sendMessage(*clients[clientSocket], response);
            // reply with SERVERS
        }
    }
    else if (tokens[0].compare("SERVERS") == 0)
    {
        cout << "Revieved SERVERS" << endl;
        // SERVERS,A5_1,130.208.243.61,8888;A5_2,10.2.132.12,10042;
        // handle the list of servers
        if (tokens.size() < 2)
        {
            return;
        }
        cout << "Client name: " << clients[clientSocket]->name << endl;
        std::string loggedMessage = "Received list of servers from " + clients[clientSocket]->name;
        logMessage(loggedMessage, "");
        for (int i = 4; i < tokens.size(); i += 3)
        {
            cout << i << endl;
            cout << tokens[i] << " " << tokens[i + 1] << " " << tokens[i + 2] << endl;
            Client *server = new Client(-1);
            server->name = tokens[i];
            server->ip_address = tokens[i + 1];
            server->port = std::stoi(tokens[i + 2]);
            clients[clientSocket]->servers.push_back(server);
        }
    }
    else if (tokens[0].compare("KEEPALIVE") == 0 && connectedClient(clientSocket, clients))
    {
        // KEEPALIVE,<No. of Messages>
        std::string msg = "Received keepalive message from " + clients[clientSocket]->name + " at " + clients[clientSocket]->ip_address + ":" + std::to_string(clients[clientSocket]->port) + " with " + tokens[1] + " messages";
        logMessage(msg, "");
        if (std::stoi(tokens[1]) > 0)
        {
            // send GETMSGS to client
            logMessage("Sending GETMSGS to " + clients[clientSocket]->name + " at " + clients[clientSocket]->ip_address + ":" + std::to_string(clients[clientSocket]->port), "");
            sendMessage(clientSocket, "GETMSGS,A5_42");
        }
    }
    else if (tokens[0].compare("GETMSGS") == 0 && tokens.size() == 2 && connectedClient(clientSocket, clients))
    {
        // GETMSGS,<GROUP ID>
        // check if groupid is valid
        std::string msg = "Getting message from group number " + tokens[1] + "requested by: " + clients[clientSocket]->name + " at " + clients[clientSocket]->ip_address + ":" + std::to_string(clients[clientSocket]->port);
        logMessage(msg, "");
        for (auto message : messageVector)
        {
            if (message.to.compare(tokens[1]) == 0)
            {
                std::string response = "SENDMSG," + message.to + "," + message.from + "," + message.message;
                logMessage("Sending message to " + clients[clientSocket]->name + " at " + clients[clientSocket]->ip_address + ":" + std::to_string(clients[clientSocket]->port) + " from group " + message.from, "");
                sendMessage(*clients[clientSocket], response);
            }
        }
    }
    else if (tokens[0].compare("SENDMSG") == 0 && tokens.size() == 4 && connectedClient(clientSocket, clients))
    {

        if (tokens[1] == "A5_42")
        {
            logMessage("Received message from " + tokens[2] + " to group number " + tokens[1], "");
            logMessage("Received message from " + tokens[2] + " Message Content:" + tokens[3], "Message_log.txt");
        }
        else
        {
            logMessage("Received message from " + tokens[2] + " to group number " + tokens[1], "");
            std::time_t now = std::time(0);
            char timeStr[100];
            strftime(timeStr, sizeof(timeStr), "%d-%m-%Y", localtime(&now));
            Message message = Message(tokens[3], tokens[2], tokens[1], timeStr);
            messageVector.push_back(message);
            if (messageMap.find(tokens[1]) == messageMap.end())
            {
                messageMap[tokens[1]] = 1;
            }
            else
            {
                messageMap[tokens[1]]++;
            }
        }
    }
    else if (tokens[0].compare("STATUSREQ") == 0 && connectedClient(clientSocket, clients))
    {
        // reply with STATUSRESP
        logMessage("Received status request from " + clients[clientSocket]->name + " at " + clients[clientSocket]->ip_address + ":" + std::to_string(clients[clientSocket]->port), "");
        std::string response = "STATUSRESP,";
        for (auto message : messageMap)
        {
            response += message.first + "," + std::to_string(message.second) + ",";
        }
        logMessage("Sending status response to " + clients[clientSocket]->name + " at " + clients[clientSocket]->ip_address + ":" + std::to_string(clients[clientSocket]->port), "");
        sendMessage(*clients[clientSocket], response);
    }
    else if (tokens[0].compare("STATUSRESP") == 0 && tokens.size() >= 1 && connectedClient(clientSocket, clients))
    {
        Client *server = clients[clientSocket];
        logMessage("Received status response from " + server->name + " at " + server->ip_address + ":" + std::to_string(server->port), "");
        for (int i = 1; i < tokens.size(); i += 2)
        {
            if (tokens[i] == "A5_42")
            {
                logMessage("Sending GETMSGS to " + server->name + " at " + server->ip_address + ":" + std::to_string(server->port) + " for " + tokens[i + 1], "");
                sendMessage(clientSocket, "GETMSGS,A5_42");
            }
            else
            {
                for (auto client : clients)
                {
                    if (client.second->name == tokens[i])
                    {
                        logMessage("Sending GETMSGS to " + server->name + " at " + server->ip_address + ":" + std::to_string(server->port) + " for " + tokens[i + 1], "");
                        sendMessage(clientSocket, ("GETMSGS," + tokens[i]).c_str());
                    }
                }
            }
        }
    }
    // From client
    else if (tokens[0].compare("SENDMSG") == 0 && tokens.size() == 3 && main_client == clientSocket)
    {
        // NOTE: if you dont know this group, forward to the groups you know and let them handle it
        std::string sanitizedToken2 = tokens[2];
        size_t pos = sanitizedToken2.find('\n');
        if (pos != std::string::npos)
        {
            sanitizedToken2.erase(pos, 1);
        }

        std::string msg = "Sending '" + sanitizedToken2 + "' to group number " + tokens[1];
        logMessage(msg, "");
        send(clientSocket, msg.c_str(), msg.length(), 0);
    }
    else if (tokens[0].compare("GETMSG") == 0 && tokens.size() == 2 && main_client == clientSocket)
    {
        std::string msg = "Getting message from group number " + tokens[1];
        logMessage(msg, "");
        send(clientSocket, msg.c_str(), msg.length(), 0);
    }
    else if (tokens[0].compare("LISTSERVERS") == 0 && main_client == clientSocket)
    {
        std::string msg = "Listing all servers we are connected to: ";
        ;
        for (auto server : clients)
        {
            msg += server.second->name + ": " + std::to_string(server.second->sock) + ", ";
        }
        logMessage(msg, "");
        send(clientSocket, msg.c_str(), msg.length(), 0);
    }
    else
    {
        std::string msg = "Invalid command from " + clients[clientSocket]->name + " at " + clients[clientSocket]->ip_address + ":" + std::to_string(clients[clientSocket]->port);
        logMessage(msg, "");
        clients[clientSocket]->misbehaveCounter++;
        return;
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

void firstConnection(int sock, vector<Client *> &clients)
{
    // iterate through client list and connect with at least 3 clients
    for (int i = 1; i < 4; i++)
    {
        Client client = *clients[i];
        // create socket for client
        int tempSock = open_socket(client.port, client.ip_address);
        if (tempSock < 0)
        {
            perror("Failed to open socket");
            exit(1);
        }
        // send message to client
        sendMessage(client, "HELO,A5_42");
        logMessage("Sending HELO,A5_42 to " + client.name + " at " + client.ip_address + ":" + to_string(client.port), "");
        // receive message from client
        char buffer[5000];
        int bytesRecieved = recv(tempSock, buffer, sizeof(buffer), 0);
        if (bytesRecieved > 0)
        {
            // Create a new client entry in the clients map
            clients.push_back(new Client(tempSock));
            clients.back()->ip_address = client.ip_address;
            clients.back()->port = client.port;
            clients.back()->heloSent = true;
            clientCommand(tempSock, buffer);
            // Add new client to the pollfds vector
            struct pollfd newClientPollFD;
            newClientPollFD.fd = tempSock;
            newClientPollFD.events = POLLIN; // We want to read from this socket
            pollfds.push_back(newClientPollFD);
        }
        else
        {
            printf("Failed to receive message from client\n");
        }
        memccpy(buffer, "", sizeof(buffer), sizeof(buffer));
        bytesRecieved = recv(tempSock, buffer, sizeof(buffer), 0);
        clientCommand(tempSock, buffer);
    }
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
    listenSock = open_socket(atoi(argv[1]), "130.208.246.249");

    if (listen(listenSock, BACKLOG) < 0)
    {
        printf("Listen failed on port %s\n", argv[1]);
        exit(0);
    }
    else
    {
        printf("Listening on port: %d\n", atoi(argv[1]));
    }

    // Add listening socket to the pollfds vector
    struct pollfd listenPollFD;
    listenPollFD.fd = listenSock;
    listenPollFD.events = POLLIN; // We are interested in when there's incoming connection
    pollfds.push_back(listenPollFD);

    // establish minimum connections to begin server
    int firstSock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("130.208.246.249");
    server_addr.sin_port = htons(5001);
    printf("Connecting to server\n");

    if (connect(firstSock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Failed to connect to server");
        close(firstSock);
        exit(1);
    }

    Client firstClient = Client(firstSock);
    firstClient.ip_address = "130.208.246.249";
    firstClient.port = 5001;
    logMessage("Sending HELO,A5_42 to port 5001", "");
    sendMessage(firstClient, "HELO,A5_42");
    printf("Sending Helo to server\n");
    int bytesRecieved;
    if ((bytesRecieved = recv(firstSock, buffer, sizeof(buffer), 0)) > 0)
    {
        // Create a new client entry in the clients map and add to pollfds
        clients[firstSock] = new Client(firstSock);
        clients[firstSock]->ip_address = "130.208.246.249";
        clients[firstSock]->port = 5001;
        cout << "First ip and port: " << clients[firstSock]->ip_address << " : " << clients[firstSock]->port << endl;
        clients[firstSock]->heloSent = true;
        clientCommand(firstSock, buffer);
        cout << "Added client " << clients[firstSock]->name << " to clients map\n";

        // Add new client to the pollfds vector
        struct pollfd newClientPollFD;
        newClientPollFD.fd = firstSock;
        newClientPollFD.events = POLLIN | POLLOUT; // We want to read from this socket
        pollfds.push_back(newClientPollFD);
        char secondBuffer[5000];
        memccpy(buffer, "", sizeof(secondBuffer), sizeof(secondBuffer));
        cout << "Getting second response" << endl;
        if (bytesRecieved = recv(firstSock, secondBuffer, sizeof(secondBuffer), 0) > 0)
        {
            cout << "Received response from server\n";
            clientCommand(firstSock, secondBuffer);
        }
        else
        {
            cout << "Failed to receive message from client\n";
        }
    }
    else
    {
        printf("Failed to receive message from client\n");
    }
    cout << "Finished!" << endl;
}
