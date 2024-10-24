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

// void clientCommand(int clientSocket, char *buffer)
// {
//     if (buffer == NULL)
//     {
//         std::string msg = "Invalid command from " + clients[clientSocket]->name + " at " + clients[clientSocket]->ip_address + " : " + std::to_string(clients[clientSocket]->port);
//         logMessage(msg, "");
//         clients[clientSocket]->misbehaveCounter++;
//         return;
//     }
//     std::vector<std::vector<std::string>> all_tokens = checkMessageContentAndProcess(buffer);
// }

void clientCommand(int clientSocket, char *buffer)
{
    handleInvalidCommand(clientSocket, buffer);

    std::vector<std::vector<std::string>> all_tokens = checkMessageContentAndProcess(buffer);

    for (auto &tokens : all_tokens)
    {
        dispatchCommand(clientSocket, tokens);
    }
}

void dispatchCommand(int clientSocket, const std::vector<std::string> &tokens)
{
    if (tokens[0] == "Rattatoskur")
    {
        handleRattatoskurCommand(clientSocket);
    }
    else if (tokens[0] == "HELO")
    {
        handleHeloCommand(clientSocket, tokens);
    }
    else if (tokens[0] == "SERVERS")
    {
        handleServersCommand(clientSocket, tokens);
    }
    else if (tokens[0] == "KEEPALIVE")
    {
        handleKeepAliveCommand(clientSocket, tokens);
    }
    else if (tokens[0] == "LISTSERVERS")
    {
        handleListServersCommand(clientSocket);
    }
    else if (tokens[0] == "SENDMSG")
    {
        handleSendMsgCommand(clientSocket, tokens);
    }
    else if (tokens[0] == "STATUSREQ")
    {
        handleStatusReqCommand(clientSocket);
    }
    else if (tokens[0] == "STATUSRESP")
    {
        handleStatusRespCommand(clientSocket, tokens);
    }
    else if (tokens[0] == "GETMSGS")
    {
        handleGetMsgsCommand(clientSocket, tokens);
    }
    else
    {
        handleInvalidCommand(clientSocket, NULL);
        clients[clientSocket]->misbehaveCounter++;
    }
}

std::vector<std::vector<std::string>> processTokens(char *buffer)
{
    return checkMessageContentAndProcess(buffer);
}

void handleRattatoskurCommand(int clientSocket)
{
    std::string msg = "Main Client connected to Server";
    logMessage(msg, "");
    send(clientSocket, msg.c_str(), msg.length(), 0);
    main_client = clientSocket;
}

void handleHeloCommand(int clientSocket, const std::vector<std::string> &tokens)
{
    if (tokens.size() == 2 && valid_id(tokens[1], clients))
    {
        logMessage("|| HELO," + tokens[1] + " || received", "");
        clients[clientSocket]->lastMessage = time(0);
        if (clients[clientSocket]->name != tokens[1])
        {
            clients[clientSocket]->name = tokens[1];
        }
        // Construct the SERVERS response and handle the HELO sent flag
        std::string response = "SERVERS,A5_42,130.208.246.249,4042;";
        sendMessage(*clients[clientSocket], response);
    }
}

void handleServersCommand(int clientSocket, const std::vector<std::string> &tokens)
{
    if (tokens.size() >= 2)
    {
        if (clients[clientSocket]->name != tokens[i] && clients[clientSocket]->ip_address != tokens[i + 1] && clients[clientSocket]->port != std::stoi(tokens[i + 2]))
        {
            logMessage("|| ERROR || " + clients[clientSocket]->name + " at " + clients[clientSocket]->ip_address + " : " + std::to_string(clients[clientSocket]->port), "");
            logMessage("\tServer did not send themselves as the first server", "");
            clients[clientSocket]->misbehaveCounter++;
            return;
        }
        logMessage("|| SERVERS || received", "");
        clients[clientSocket]->lastMessage = time(0);
        for (size_t i = 4; i < tokens.size(); i += 3)
        {
            Client *server = new Client(-1);
            server->name = tokens[i];
            server->ip_address = tokens[i + 1];
            server->port = std::stoi(tokens[i + 2]);
            clients[clientSocket]->servers.push_back(server);
        }
    }
}

void handleKeepAliveCommand(int clientSocket, const std::vector<std::string> &tokens)
{
    if (tokens.size() == 2 && std::stoi(tokens[1]) > 0)
    {
        std::string msg = "|| KEEPALIVE || received from " + clients[clientSocket]->name + " at " + clients[clientSocket]->ip_address + " : " + std::to_string(clients[clientSocket]->port) + " with " + tokens[1] + " messages";
        logMessage(msg, "");
        clients[clientSocket]->lastMessage = time(0);
        if (std::stoi(tokens[1]) > 0)
        {
            // send GETMSGS to client
            sendMessage(clientSocket, "GETMSGS,A5_42");
            logMessage("|| GETMSGS || sent to " + clients[clientSocket]->name + " at " + clients[clientSocket]->ip_address + " : " + std::to_string(clients[clientSocket]->port), "");
        }
    }
}

void handleListServersCommand(int clientSocket)
{
    std::string msg = "Listing all servers we are connected to: ";
    for (const auto &server : clients)
    {
        msg += server.second->name + ": " + std::to_string(server.second->sock) + ", ";
    }
    logMessage(msg, "client.log");
    send(clientSocket, msg.c_str(), msg.length(), 0);
}

void handleSendMsgCommand(int clientSocket, const std::vector<std::string> &tokens)
{
    if (tokens.size() == 4 && connectedClient(clientSocket, clients))
    {
        logMessage("|| SENDMSG || received from " + tokens[2] + " to group number " + tokens[1], "");
        clients[clientSocket]->lastMessage = time(0);

        if (tokens[1] == "A5_42")
        {
            logMessage("Received message from " + tokens[2] + " Message Content:" + tokens[3], "Message_log.txt");
        }

        std::time_t now = std::time(0);
        char timeStr[100];
        strftime(timeStr, sizeof(timeStr), "%d-%m-%Y", localtime(&now));
        Message message(tokens[3], tokens[2], tokens[1], timeStr);
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

void handleStatusReqCommand(int clientSocket)
{
    logMessage("|| STATUSREQ || received from " + clients[clientSocket]->name + " at " + clients[clientSocket]->ip_address + " : " + std::to_string(clients[clientSocket]->port), "");
    clients[clientSocket]->lastMessage = time(0);

    std::string response = "STATUSRESP,";
    for (const auto &message : messageMap)
    {
        response += message.first + "," + std::to_string(message.second) + ",";
    }

    sendMessage(*clients[clientSocket], response);
    logMessage("|| STATUSRESP || sent to " + clients[clientSocket]->name + " at " + clients[clientSocket]->ip_address + " : " + std::to_string(clients[clientSocket]->port), "");
}

void handleStatusRespCommand(int clientSocket, const std::vector<std::string> &tokens)
{
    logMessage("|| STATUSRESP || received from " + clients[clientSocket]->name + " at " + clients[clientSocket]->ip_address + " : " + std::to_string(clients[clientSocket]->port), "");
    clients[clientSocket]->lastMessage = time(0);

    for (size_t i = 1; i < tokens.size(); i += 2)
    {
        if (tokens[i] == "A5_42")
        {
            sendMessage(clientSocket, "GETMSGS,A5_42");
            logMessage("|| GETMSGS || sent to " + clients[clientSocket]->name + " at " + clients[clientSocket]->ip_address + " : " + std::to_string(clients[clientSocket]->port) + " for " + tokens[i + 1], "");
        }
        else
        {
            for (const auto &client : clients)
            {
                if (client.second->name == tokens[i])
                {
                    sendMessage(clientSocket, ("GETMSGS," + tokens[i]).c_str());
                    logMessage("|| GETMSGS || sent to " + clients[clientSocket]->name + " at " + clients[clientSocket]->ip_address + " : " + std::to_string(clients[clientSocket]->port) + " for " + tokens[i + 1], "");
                }
            }
        }
    }
}

void handleGetMsgsCommand(int clientSocket, const std::vector<std::string> &tokens)
{
    if (tokens.size() == 2 && connectedClient(clientSocket, clients))
    {
        std::string msg = "|| GETMSGS || received from group number " + tokens[1] + " requested by: " + clients[clientSocket]->name + " at " + clients[clientSocket]->ip_address + " : " + std::to_string(clients[clientSocket]->port);
        logMessage(msg, "");

        clients[clientSocket]->lastMessage = time(0);
        for (const auto &message : messageVector)
        {
            if (message.to == tokens[1])
            {
                std::string response = "SENDMSG," + message.to + "," + message.from + "," + message.message;
                sendMessage(*clients[clientSocket], response);
                logMessage("|| SENDMSG || sent to " + clients[clientSocket]->name + " at " + clients[clientSocket]->ip_address + " : " + std::to_string(clients[clientSocket]->port) + " from group " + message.from, "");
            }
        }
    }
}

void handleInvalidCommand(int clientSocket, char *buffer)
{
    if (buffer == NULL)
    {
        std::string msg = "Invalid command from " + clients[clientSocket]->name + " at " + clients[clientSocket]->ip_address + " : " + std::to_string(clients[clientSocket]->port);
        logMessage(msg, "");
        clients[clientSocket]->misbehaveCounter++;
    }
}

// for (auto tokens : all_tokens)
// {
//     if (tokens[0].compare("Rattatoskur") == 0)
//     {
//         std::string msg = "Main Client connected to Server";
//         logMessage(msg, "");
//         send(clientSocket, msg.c_str(), msg.length(), 0);
//         main_client = clientSocket;
//     }
//     else if (tokens[0].compare("HELO") == 0 && tokens.size() == 2)
//     {
//         logMessage("|| HELO," + tokens[1] + " || recieved from " + tokens[1] + " at " + clients[clientSocket]->ip_address + " : " + std::to_string(clients[clientSocket]->port), "");
//         clients[clientSocket]->lastMessage = time(0);
//         if (valid_id(tokens[1], clients))
//         {
//             if (clients[clientSocket]->name == tokens[1])
//             {
//                 return;
//             }
//             clients[clientSocket]->name = tokens[1];
//             std::string response = "SERVERS,A5_42,130.208.246.249,4042;";
//             for (auto server : clients)
//             {
//                 if (server.second->name != clients[clientSocket]->name)
//                 {
//                     response += server.second->name + "," + server.second->ip_address + "," + std::to_string(server.second->port) + ";";
//                 }
//             }
//             if (!clients[clientSocket]->heloSent)
//             {
//                 clients[clientSocket]->heloSent = true;
//                 sendMessage(*clients[clientSocket], "HELO,A5_42");
//                 logMessage("|| HELO,A5_42 || sent to " + clients[clientSocket]->name + " at " + clients[clientSocket]->ip_address + " : " + std::to_string(clients[clientSocket]->port), "");
//             }
//             sendMessage(*clients[clientSocket], response);
//             logMessage("|| SERVERS || sent to " + clients[clientSocket]->name + " at " + clients[clientSocket]->ip_address + " : " + std::to_string(clients[clientSocket]->port), "");
//             // reply with SERVERS
//         }
//     }
//     else if (tokens[0].compare("SERVERS") == 0)
//     {
//         if (tokens.size() < 2)
//         {
//             return;
//         }
//         std::string loggedMessage = "|| SERVERS || received from " + clients[clientSocket]->name + " at " + clients[clientSocket]->ip_address + " : " + std::to_string(clients[clientSocket]->port);
//         logMessage(loggedMessage, "");
//         clients[clientSocket]->lastMessage = time(0);
//         // for (int i = 1; i < 4; i++)
//         // {
//         //     if (clients[clientSocket]->name != tokens[i] && clients[clientSocket]->ip_address != tokens[i + 1] && clients[clientSocket]->port != std::stoi(tokens[i + 2]))
//         //     {
//         //         logMessage("|| ERROR || " + clients[clientSocket]->name + " at " + clients[clientSocket]->ip_address + " : " + std::to_string(clients[clientSocket]->port), "");
//         //         logMessage("\tServer did not send themselves as the first server", "");
//         //         clients[clientSocket]->misbehaveCounter++;
//         //         return;
//         //     }
//         // }
//         for (size_t i = 4; i < tokens.size(); i += 3)
//         {
//             Client *server = new Client(-1);
//             server->name = tokens[i];
//             server->ip_address = tokens[i + 1];
//             server->port = std::stoi(tokens[i + 2]);
//             clients[clientSocket]->servers.push_back(server);
//         }
//     }
//     else if (tokens[0].compare("KEEPALIVE") == 0 && connectedClient(clientSocket, clients))
//     {
//         // KEEPALIVE,<No. of Messages>
//         std::string msg = "|| KEEPALIVE || received from " + clients[clientSocket]->name + " at " + clients[clientSocket]->ip_address + " : " + std::to_string(clients[clientSocket]->port) + " with " + tokens[1] + " messages";
//         logMessage(msg, "");
//         clients[clientSocket]->lastMessage = time(0);
//         if (std::stoi(tokens[1]) > 0)
//         {
//             // send GETMSGS to client
//             sendMessage(clientSocket, "GETMSGS,A5_42");
//             logMessage("|| GETMSGS || sent to " + clients[clientSocket]->name + " at " + clients[clientSocket]->ip_address + " : " + std::to_string(clients[clientSocket]->port), "");
//         }
//     }
//     else if (tokens[0].compare("GETMSGS") == 0 && tokens.size() == 2 && connectedClient(clientSocket, clients))
//     {
//         // GETMSGS,<GROUP ID>
//         // check if groupid is valid
//         std::string msg = "|| GETMSGS || received from group number " + tokens[1] + "requested by: " + clients[clientSocket]->name + " at " + clients[clientSocket]->ip_address + " : " + std::to_string(clients[clientSocket]->port);
//         logMessage(msg, "");
//         clients[clientSocket]->lastMessage = time(0);
//         for (auto message : messageVector)
//         {
//             if (message.to.compare(tokens[1]) == 0)
//             {
//                 std::string response = "SENDMSG," + message.to + "," + message.from + "," + message.message;
//                 sendMessage(*clients[clientSocket], response);
//                 logMessage("|| SENDMSG || sent to " + clients[clientSocket]->name + " at " + clients[clientSocket]->ip_address + " : " + std::to_string(clients[clientSocket]->port) + " from group " + message.from, "");
//             }
//         }
//     }
//     else if (tokens[0].compare("SENDMSG") == 0 && tokens.size() == 4 && connectedClient(clientSocket, clients))
//     {

//         logMessage("|| SENDMSG || received from " + tokens[2] + " to group number " + tokens[1], "");
//         clients[clientSocket]->lastMessage = time(0);
//         if (tokens[1] == "A5_42")
//         {
//             // also add to messageVector, we will use GETMSGS from client to get them
//             logMessage("Received message from " + tokens[2] + " Message Content:" + tokens[3], "Message_log.txt");
//         }
//         std::time_t now = std::time(0);
//         char timeStr[100];
//         strftime(timeStr, sizeof(timeStr), "%d-%m-%Y", localtime(&now));
//         Message message = Message(tokens[3], tokens[2], tokens[1], timeStr);
//         messageVector.push_back(message);
//         if (messageMap.find(tokens[1]) == messageMap.end())
//         {
//             messageMap[tokens[1]] = 1;
//         }
//         else
//         {
//             messageMap[tokens[1]]++;
//         }
//     }
//     else if (tokens[0].compare("STATUSREQ") == 0 && connectedClient(clientSocket, clients))
//     {
//         // reply with STATUSRESP
//         logMessage("|| STATUSREQ || received from " + clients[clientSocket]->name + " at " + clients[clientSocket]->ip_address + " : " + std::to_string(clients[clientSocket]->port), "");
//         clients[clientSocket]->lastMessage = time(0);
//         std::string response = "STATUSRESP,";
//         for (auto message : messageMap)
//         {
//             response += message.first + "," + std::to_string(message.second) + ",";
//         }
//         sendMessage(*clients[clientSocket], response);
//         logMessage("|| STATUSRESP || sent to " + clients[clientSocket]->name + " at " + clients[clientSocket]->ip_address + " : " + std::to_string(clients[clientSocket]->port), "");
//     }
//     else if (tokens[0].compare("STATUSRESP") == 0 && tokens.size() >= 1 && connectedClient(clientSocket, clients))
//     {
//         logMessage("|| STATUSRESP || received from " + clients[clientSocket]->name + " at " + clients[clientSocket]->ip_address + " : " + std::to_string(clients[clientSocket]->port), "");
//         clients[clientSocket]->lastMessage = time(0);
//         for (size_t i = 1; i < tokens.size(); i += 2)
//         {
//             if (tokens[i] == "A5_42")
//             {
//                 sendMessage(clientSocket, "GETMSGS,A5_42");
//                 logMessage("|| GETMSGS || sent to " + clients[clientSocket]->name + " at " + clients[clientSocket]->ip_address + " : " + std::to_string(clients[clientSocket]->port) + " for " + tokens[i + 1], "");
//             }
//             else
//             {
//                 for (auto client : clients)
//                 {
//                     if (client.second->name == tokens[i])
//                     {
//                         sendMessage(clientSocket, ("GETMSGS," + tokens[i]).c_str());
//                         logMessage("|| GETMSGS || sent to " + clients[clientSocket]->name + " at " + clients[clientSocket]->ip_address + " : " + std::to_string(clients[clientSocket]->port) + " for " + tokens[i + 1], "");
//                     }
//                 }
//             }
//         }
//     }

//     // From client
//     else if (tokens[0].compare("SENDMSG") == 0 && tokens.size() == 3 && main_client == clientSocket)
//     {
//         // NOTE: if you dont know this group, forward to the groups you know and let them handle it
//         std::string sanitizedToken2 = tokens[2];
//         size_t pos = sanitizedToken2.find('\n');
//         if (pos != std::string::npos)
//         {
//             sanitizedToken2.erase(pos, 1);
//         }

//         std::string msg = "Sending '" + sanitizedToken2 + "' to group number " + tokens[1];
//         logMessage(msg, "client.log");
//         send(clientSocket, msg.c_str(), msg.length(), 0);
//     }
//     else if (tokens[0].compare("GETMSG") == 0 && tokens.size() == 2 && main_client == clientSocket)
//     {
//         std::string msg = "Getting message from group number " + tokens[1];
//         logMessage(msg, "client.log");
//         send(clientSocket, msg.c_str(), msg.length(), 0);
//     }
//     else if (tokens[0].compare("LISTSERVERS") == 0 && main_client == clientSocket)
//     {
//         std::string msg = "Listing all servers we are connected to: ";
//         ;
//         for (auto server : clients)
//         {
//             msg += server.second->name + ": " + std::to_string(server.second->sock) + ", ";
//         }
//         logMessage(msg, "client.log");
//         send(clientSocket, msg.c_str(), msg.length(), 0);
//     }
//     else
//     {
//         logMessage("|| ERROR || " + clients[clientSocket]->name + " at " + clients[clientSocket]->ip_address + " : " + std::to_string(clients[clientSocket]->port), "");
//         logMessage("\tInvalid command : " + tokens[0], "");
//         std::string msg = "Invalid command from " + clients[clientSocket]->name + " at " + clients[clientSocket]->ip_address + " : " + std::to_string(clients[clientSocket]->port);
//         logMessage(msg, "");
//         clients[clientSocket]->misbehaveCounter++;
//         return;
//     }
// }

// Remove fd_set and declare a vector of pollfd instead
std::vector<struct pollfd> pollfds;

void closeClient(int clientSocket, std::vector<struct pollfd> &pollfds)
{
    printf("Client closed connection: %d\n", clientSocket);
    // delete the client from clients
    // use destructor
    delete clients[clientSocket];
    clients.erase(clientSocket);
    close(clientSocket);
    // Remove the socket from the pollfds vector
    auto it = std::remove_if(pollfds.begin(), pollfds.end(),
                             [clientSocket](struct pollfd &pfd)
                             {
                                 return pfd.fd == clientSocket;
                             });
    pollfds.erase(it, pollfds.end());
}

void firstConnection(vector<Client *> &client_vector)
{
    char buffer[5000];
    printf("FirstConnection started\n");
    for (auto client : client_vector)
    {
        if (client->name.find("A5_") != string::npos || client->name.find("Instr_") != string::npos)
        {
            if (client->name == "A5_42" || client->name == "Instr_2" || client->name == "Instr_1" || client->port == -1)
            {
                continue;
            }
            else
            {
                int connectionSocket = socket(AF_INET, SOCK_STREAM, 0);
                struct sockaddr_in server_addr;
                server_addr.sin_family = AF_INET;
                server_addr.sin_addr.s_addr = inet_addr(client->ip_address.c_str());
                server_addr.sin_port = htons(client->port);
                cout << "Trying to connect to " << client->name << " at " << client->ip_address << ":" << client->port << endl;

                if (connect(connectionSocket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
                {
                    perror("Failed to connect to server");
                    close(connectionSocket);
                    exit(1);
                }

                Client firstClient = Client(connectionSocket);
                firstClient.ip_address = client->ip_address;
                firstClient.port = client->port;
                std::string msg = "Sending HELO,A5_42 to client " + client->name + " at " + client->ip_address + " : " + std::to_string(client->port);
                logMessage(msg, "");
                sendMessage(firstClient, "HELO,A5_42");
                printf("Sending Helo to server\n");
                int bytesRecieved;
                if ((bytesRecieved = recv(connectionSocket, buffer, sizeof(buffer), 0)) > 0)
                {
                    cout << "Received message from " << firstClient.name << endl;
                    // Create a new client entry in the clients map and add to pollfds
                    clients[connectionSocket] = new Client(connectionSocket);
                    clients[connectionSocket]->ip_address = firstClient.ip_address;
                    clients[connectionSocket]->port = firstClient.port;
                    clients[connectionSocket]->heloSent = true;
                    clients[connectionSocket]->name = firstClient.name;
                    clientCommand(connectionSocket, buffer);
                    // Add new client to the pollfds vector
                    struct pollfd newClientPollFD;
                    newClientPollFD.fd = connectionSocket;
                    newClientPollFD.events = POLLIN | POLLOUT; // We want to read from this socket
                    pollfds.push_back(newClientPollFD);
                    memccpy(buffer, "", sizeof(buffer), sizeof(buffer));
                    if (bytesRecieved = recv(connectionSocket, buffer, sizeof(buffer), 0) > 0)
                    {
                        clientCommand(connectionSocket, buffer);
                    }
                }
            }
        }
        else
        {
            printf("Invalid server name\n");
        }
    }
}

int main(int argc, char *argv[])
{
    bool finished = false;
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
    // while (clients.size() != MINSERVERS)
    // {
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
    printf("Connected to server\n");
    firstClient.port = 5001;
    logMessage("Sending HELO,A5_42 to port 5001", "");
    sendMessage(firstClient, "HELO,A5_42");
    int bytesRecieved;
    if ((bytesRecieved = recv(firstSock, buffer, sizeof(buffer), 0)) > 0)
    {
        // Create a new client entry in the clients map and add to pollfds
        clients[firstSock] = new Client(firstSock);
        clients[firstSock]->ip_address = firstClient.ip_address;
        clients[firstSock]->port = firstClient.port;
        clients[firstSock]->heloSent = true;
        clientCommand(firstSock, buffer);
        cout << "Added client " << clients[firstSock]->name << " to clients map\n";
        // Add new client to the pollfds vector
        struct pollfd newClientPollFD;
        newClientPollFD.fd = firstSock;
        newClientPollFD.events = POLLIN | POLLOUT;
        pollfds.push_back(newClientPollFD);
        char secondBuffer[5000];
        memccpy(buffer, "", sizeof(secondBuffer), sizeof(secondBuffer));
        if (bytesRecieved = recv(firstSock, secondBuffer, sizeof(secondBuffer), 0) > 0)
        {
            clientCommand(firstSock, secondBuffer);
            cout << "Finished connecting to Instr_1" << endl;
            firstConnection(clients[firstSock]->servers);
        }
        else
        {
            printf("Failed to receive second message from client\n");
        }
    }
    else
    {
        printf("Failed to receive message from client\n");
    }

    std::map<int, time_t> last_message_time; // Track last message received for each connection
    time_t last_keepalive_time = time(nullptr);
    time_t keepalive_interval = 60;   // Send keepalive every minute
    time_t disconnect_interval = 300; // 5 minutes timeout
    int timeout = 5000;               // 5 seconds poll timeout

    char recieveBuffer[5000];
    while (!finished)
    {
        time_t current_time = time(nullptr);

        if (current_time - last_keepalive_time >= keepalive_interval)
        {
            for (auto poll : pollfds)
            {
                sendKeepalive(*clients[poll.fd], messageMap[clients[poll.fd]->name]);
            }
        }
        last_keepalive_time = current_time;

        // poll
        int ret = poll(pollfds.data(), pollfds.size(), timeout);

        if (ret > 0)
        {
            for (auto &fd : pollfds)
            {
                if (fd.revents & POLLIN)
                {
                    // needs handling incoming data
                    char recievedBuffer[5000];
                    recv(fd.fd, recieveBuffer, sizeof(recieveBuffer), 0);
                    if (recieveBuffer == NULL)
                    {
                        if (clients[fd.fd]->misbehaveCounter >= MAXMISBEHAVIOUR)
                        {
                            closeClient(fd.fd, pollfds);
                            continue;
                        }
                        else if (last_message_time[fd.fd] > 300)
                        {
                            closeClient(fd.fd, pollfds);
                            continue;
                        }
                    }
                    else
                    {
                        clientCommand(fd.fd, recieveBuffer);
                        last_message_time[fd.fd] = current_time;
                    }
                }
            }
        }

        for (auto it = pollfds.begin(); it != pollfds.end();)
        {
            if (current_time - last_message_time[it->fd] >= disconnect_interval)
            {
                closeClient(it->fd, pollfds);
                it = pollfds.erase(it);
            }
            else
            {
                ++it;
            }
        }
        // make sure there are at least 3 connections
        while (pollfds.size() <= MINSERVERS)
        {
            // choose random client servers from client
            //  connect to them

            firstConnection(clients[rand() % clients.size()]->servers);
        }
    }
}
