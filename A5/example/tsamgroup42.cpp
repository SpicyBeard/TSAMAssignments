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
#include <random>
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
std::vector<struct pollfd> pollfds; // vector of pollfd instead

void closeClient(int clientSocket, std::vector<struct pollfd> &pollfds)
{
    // Close the client socket
    close(clientSocket);

    // Remove the socket from the pollfds vector
    auto it = std::remove_if(pollfds.begin(), pollfds.end(),
                             [clientSocket](const struct pollfd &pfd)
                             {
                                 return pfd.fd == clientSocket;
                             });
    pollfds.erase(it, pollfds.end());
}

void removeClient(std::map<int, Client *> &clients, std::vector<struct pollfd> &pollfds, std::map<int, Client *>::iterator &it)
{
    // Check if the socket is in pollfds and is open
    auto pollfdIt = std::find_if(pollfds.begin(), pollfds.end(), [&](const struct pollfd &pfd)
                                 { return pfd.fd == it->first; });

    if (pollfdIt != pollfds.end())
    {
        // Check if the socket is open
        int error = 0;
        socklen_t len = sizeof(error);
        int retval = getsockopt(it->first, SOL_SOCKET, SO_ERROR, &error, &len);
        if (retval == 0 && error == 0)
        {
            // Socket is open, proceed to close it
            closeClient(it->first, pollfds);
        }
    }

    // Delete the client and erase from the map
    delete it->second;
    it = clients.erase(it);
}

std::vector<std::vector<std::string>> processTokens(char *buffer)
{
    return checkMessageContentAndProcess(buffer);
}

void handleRattatoskurCommand(int clientSocket)
{
    std::string msg = "Main Client connected to Server";
    logMessage(msg, "", true);
    send(clientSocket, msg.c_str(), msg.length(), 0);
    main_client = clientSocket;
}

void handleHeloCommand(int clientSocket, const std::vector<std::string> &tokens)
{
    if (tokens.size() == 2 && valid_id(tokens[1], clients))
    {
        if (tokens[1] == "A5_300")
        {
            auto it = clients.find(clientSocket);
            removeClient(clients, pollfds, it);
            return;
        }
        clients[clientSocket]->name = tokens[1];
        logMessage("|| " + tokens[0] + "," + tokens[1] + " || received from " + clients[clientSocket]->name + " at " + clients[clientSocket]->ip_address + " : " + std::to_string(clients[clientSocket]->port), "", true);
        clients[clientSocket]->lastMessage = time(0);
        clients[clientSocket]->ip_address = getSourceIpandPort(clientSocket).first;
        clients[clientSocket]->port = getSourceIpandPort(clientSocket).second;
        if (!clients[clientSocket]->heloSent)
        {
            clients[clientSocket]->heloSent = true;
            sendMessage(*clients[clientSocket], "HELO,A5_42");
            logMessage("|| HELO,A5_42 || sent to " + clients[clientSocket]->name + " at " + clients[clientSocket]->ip_address + " : " + std::to_string(clients[clientSocket]->port), "", true);
        }
        // Construct the SERVERS response and handle the HELO sent flag
        std::string response = "SERVERS,A5_42,130.208.246.249,4042;";
        for (const auto &server : clients)
        {
            if (server.second->sock != main_client && !server.second->name.empty())
            {
                response += server.second->name + "," + server.second->ip_address + "," + std::to_string(server.second->port) + ";";
            }
        }
        sendMessage(*clients[clientSocket], response);
        logMessage("|| SERVERS || sent to " + clients[clientSocket]->name + " at " + clients[clientSocket]->ip_address + " : " + std::to_string(clients[clientSocket]->port), "", true);
    }
}

void handleServersCommand(int clientSocket, const std::vector<std::string> &tokens)
{
    if (tokens[1] != "A5_22")
    {

        if (tokens.size() >= 2)
        {
            clients[clientSocket]->lastMessage = time(0);

            logMessage("|| SERVERS || received from " + clients[clientSocket]->name + " at " + clients[clientSocket]->ip_address + " : " + std::to_string(clients[clientSocket]->port), "", true);

            if (clients[clientSocket]->name != tokens[1])
            {
                logMessage("|| ERROR || " + clients[clientSocket]->name + " at " + clients[clientSocket]->ip_address + " : " + std::to_string(clients[clientSocket]->port) + "\n\tServer did not send themselves as the first server", "", true);
                clients[clientSocket]->misbehaveCounter++;
                return;
            }

            clients[clientSocket]->servers.clear();
            clients[clientSocket]->ip_address = tokens[2];
            clients[clientSocket]->port = std::stoi(tokens[3]);

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
    clients[clientSocket]->misbehaveCounter += 1;
}

void handleKeepAliveCommand(int clientSocket, const std::vector<std::string> &tokens)
{
    if (tokens.size() == 2)
    {
        std::string msg = "|| KEEPALIVE || received from " + clients[clientSocket]->name + " at " + clients[clientSocket]->ip_address + " : " + std::to_string(clients[clientSocket]->port) + " with " + tokens[1] + " messages";
        logMessage(msg, "", true);
        clients[clientSocket]->lastMessage = time(0);
        if (std::stoi(tokens[1]) > 0)
        {
            // send GETMSGS to client
            sendMessage(clientSocket, "GETMSGS,A5_42");
            logMessage("|| GETMSGS || sent to " + clients[clientSocket]->name + " at " + clients[clientSocket]->ip_address + " : " + std::to_string(clients[clientSocket]->port), "", true);
        }
    }
}

void handleListServersCommand(int clientSocket)
{
    std::string msg = "Listing all servers we are connected to: ";
    for (const auto &server : clients)
    {
        if (server.second->sock != main_client)
        {
            msg += server.second->name + " at " + server.second->ip_address + " : " + std::to_string(server.second->port) + ", ";
        }
    }
    logMessage(msg, "client.log", true);
    send(clientSocket, msg.c_str(), msg.length(), 0);
}

void handleSendMsgCommand(int clientSocket, const std::vector<std::string> &tokens)
{
    if (tokens.size() == 3 && clientSocket == main_client)
    {
        logMessage("|| SENDMSG || received from main client to group number " + tokens[1], "", true);
        clients[clientSocket]->lastMessage = time(0);

        std::time_t now = std::time(0);
        char timeStr[100];
        strftime(timeStr, sizeof(timeStr), "%d-%m-%Y", localtime(&now));
        Message message(tokens[2], "A5_42", tokens[1], timeStr);

        // check if we have the group number in clients list
        for (auto client : clients)
        {
            if (client.second->name == tokens[1])
            {
                sendMessage(client.second->sock, tokens[0] + "," + tokens[1] + ",A5_42," + "," + tokens[2]);
                logMessage("|| SENDMSG || sent to " + client.second->name + " at " + client.second->ip_address + " : " + std::to_string(client.second->port) + " from group A4_42", "", true);
                logMessage("|| SENDMSG || sent to " + client.second->name + " at " + client.second->ip_address + " : " + std::to_string(client.second->port) + " from group A4_42", "client.log", true);
                std::string response = "Message sent to " + tokens[1];
                send(clientSocket, response.c_str(), response.length(), 0);
                return;
            }
        }
        logMessage("|| SENDMSG || storing message from " + clients[clientSocket]->name + " for " + tokens[1], "", true);
        std::string response = "Message to " + tokens[1] + " has been saved";
        send(clientSocket, response.c_str(), response.length(), 0);
        messageVector.push_back(message);
        if (messageMap.find(tokens[1]) == messageMap.end())
        {
            messageMap[tokens[1]] = 1;
        }
        else
        {
            messageMap[tokens[1]]++;
        }
        return;
    }
    else if (tokens.size() >= 4 && connectedClient(clientSocket, clients))
    {
        std::string receivedMsg = tokens[3];
        if (tokens.size() > 4)
        {
            for (size_t i = 4; i < tokens.size(); i++)
            {
                receivedMsg += "," + tokens[i];
            }
            std::vector<std::string> newTokens = tokens;
            newTokens[3] = receivedMsg;
        }
        if (tokens[1] == "A5_42")
        {
            logMessage("Received message from " + tokens[2] + " Message Content: " + receivedMsg, "messages.log", true);
        }

        logMessage("|| SENDMSG || received from " + clients[clientSocket]->name + " TO " + tokens[1] + " originally FROM " + tokens[2], "", true);
        clients[clientSocket]->lastMessage = time(0);

        // check if we have the group number in clients list
        for (auto client : clients)
        {
            if (client.second->name == tokens[1] && client.second->name != "A5_42")
            {
                sendMessage(client.second->sock, tokens[0] + "," + tokens[1] + "," + tokens[2] + "," + receivedMsg);
                logMessage("|| SENDMSG || sent to " + tokens[1] + " at " + client.second->ip_address + " : " + std::to_string(client.second->port) + " from group " + tokens[2], "", true);
                return;
            }
        }
        sendMessage(main_client, "Received message from" + tokens[3] + ". You can get it by sending GETMSG," + tokens[1]);
        logMessage("|| SENDMSG || storing message from " + clients[clientSocket]->name + " for " + tokens[1], "", true);

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
        return;
    }
}

void handleStatusReqCommand(int clientSocket)
{
    logMessage("|| STATUSREQ || received from " + clients[clientSocket]->name + " at " + clients[clientSocket]->ip_address + " : " + std::to_string(clients[clientSocket]->port), "", true);
    clients[clientSocket]->lastMessage = time(0);

    std::string response = "STATUSRESP";
    for (const auto &message : messageMap)
    {
        if (!message.first.empty() && message.first != "A5_42" && message.second > 0)
        {
            response += ',' + message.first + "," + std::to_string(message.second);
        }
    }

    sendMessage(*clients[clientSocket], response);
    logMessage("|| STATUSRESP || sent to " + clients[clientSocket]->name + " at " + clients[clientSocket]->ip_address + " : " + std::to_string(clients[clientSocket]->port), "", true);
}

void handleStatusRespCommand(int clientSocket, const std::vector<std::string> &tokens)
{
    logMessage("|| STATUSRESP || received from " + clients[clientSocket]->name + " at " + clients[clientSocket]->ip_address + " : " + std::to_string(clients[clientSocket]->port), "", true);
    clients[clientSocket]->lastMessage = time(0);

    for (size_t i = 1; i < tokens.size(); i += 2)
    {
        if (tokens[i] == "A5_42")
        {
            sendMessage(clientSocket, "GETMSGS,A5_42");
            logMessage("|| GETMSGS || sent to " + clients[clientSocket]->name + " at " + clients[clientSocket]->ip_address + " : " + std::to_string(clients[clientSocket]->port) + " for " + tokens[i + 1], "", true);
        }
        else
        {
            for (const auto &client : clients)
            {
                if (client.second->name == tokens[i])
                {
                    sendMessage(clientSocket, ("GETMSGS," + tokens[i]).c_str());
                    logMessage("|| GETMSGS || sent to " + clients[clientSocket]->name + " at " + clients[clientSocket]->ip_address + " : " + std::to_string(clients[clientSocket]->port) + " for " + tokens[i + 1], "", true);
                }
            }
        }
    }
}

void handleGetMsgsCommand(int clientSocket, const std::vector<std::string> &tokens)
{
    if (tokens.size() == 2 && connectedClient(clientSocket, clients))
    {
        logMessage("|| GETMSGS || received from group number " + tokens[1] + " requested by: " + clients[clientSocket]->name + " at " + clients[clientSocket]->ip_address + " : " + std::to_string(clients[clientSocket]->port) + "", "", true);
        clients[clientSocket]->lastMessage = time(0);
        if (messageMap[tokens[1]] > 0)
        {
            for (const auto &message : messageVector)
            {
                if (message.to == tokens[1])
                {
                    std::string response = "SENDMSG," + message.to + "," + message.from + "," + message.message;
                    sendMessage(*clients[clientSocket], response);
                    logMessage("|| SENDMSG || sent to " + clients[clientSocket]->name + " at " + clients[clientSocket]->ip_address + " : " + std::to_string(clients[clientSocket]->port) + " from group " + message.from, "", true);
                }
            }
        }
        logMessage("|| INFO || No messages found for group number " + tokens[1], "", true);
    }
}

void handleGetMsgCommand(int clientSocket, const std::vector<std::string> &tokens)
{
    // GETMSG,GROUP ID
    // Gets a single message from the server for the GROUP_ID
    if (tokens.size() == 2)
    {
        std::string msg = "|| GETMSG || getting message for group number " + tokens[1] + " requested by main client";
        logMessage(msg, "", true);
        for (const auto &message : messageVector)
        {
            if (message.to == tokens[1])
            {
                std::string response = "SENDING messsage for " + message.to + " from " + message.from + " : " + message.message;
                logMessage("|| SENDMSG || sent to main client from group " + message.from, "", true);
                logMessage("|| SENDMSG || sent to main client from group " + message.from, "client.log", true);
                send(clientSocket, response.c_str(), response.length(), 0);
                messageVector.erase(std::remove(messageVector.begin(), messageVector.end(), message), messageVector.end());
                return;
            }
        }
        std::string response = "|| INFO || No messages found for group number " + tokens[1];
        send(clientSocket, response.c_str(), response.length(), 0);
    }
}

void handleInvalidCommand(int clientSocket, string buffer)
{
    if (buffer.empty())
    {
        std::string msg = "|| ERROR ||Invalid command from " + clients[clientSocket]->name + " at " + clients[clientSocket]->ip_address + " : " + std::to_string(clients[clientSocket]->port);
        logMessage(msg, "", true);
        clients[clientSocket]->misbehaveCounter++;
    }
}

// Helper function to remove client from poll list
void removeClientFromPoll(int clientSocket)
{
    auto it = std::remove_if(pollfds.begin(), pollfds.end(), [clientSocket](struct pollfd &pfd)
                             { return pfd.fd == clientSocket; });
    pollfds.erase(it, pollfds.end());
}

void addNewClient(int clientSocket, string name, string ip, int port, bool heloSent)
{
    // Create a new client entry in the clients map and add to pollfds
    std::cout << "|| CONNECTING || conencting to new client: " + name + " at " + ip + " : " + std::to_string(port) << std::endl;
    clients[clientSocket] = new Client(clientSocket);
    clients[clientSocket]->ip_address = ip;
    clients[clientSocket]->port = port;
    clients[clientSocket]->heloSent = heloSent;
    // Add new client to the pollfds vector
    struct pollfd newClientPollFD;
    newClientPollFD.fd = clientSocket;
    newClientPollFD.events = POLLIN;
    pollfds.push_back(newClientPollFD);
}

// void connectToClient(Client *client)
// {

//     // return if the port is not in the range of 4000-4200 or 5000 to 5005
//     if ((client->port < 4000 || client->port > 4200) && (client->port < 5000 || client->port > 5005))
//     {
//         return;
//     }
//     if (!valid_id(client->name, clients))
//     {
//         return;
//     }
//     std::cout << "Connecting to " << client->name << " at " << client->ip_address << " : " << client->port << std::endl;

//     int sockfd = connectToServer(client->port, client->ip_address);
//     if (sockfd == -1)
//     {
//         return;
//     }

//     addNewClient(sockfd, client->name, client->ip_address, client->port, false);
//     // send HELO to instructor server
//     // std::cout << "Sending HELO,A5_42 to client " + client->name + " at " + client->ip_address + " : " + std::to_string(client->port) << std::endl;
//     // sendMessage(*clients[sockfd], "HELO,A5_42");
//     // logMessage("|| HELO,A5_42 || sent to " + client->name + " at " + client->ip_address + " : " + std::to_string(client->port), "", true);
//     // clients[sockfd]->heloSent = true;
//     // string response = receiveMessage(sockfd);
//     // if (response.length() > 0)
//     // {
//     //     cout << "Received message from " << client->name << endl;
//     //     cout << response << endl;
//     //     clientCommand(sockfd, response);
//     // }
//     // cout << "Trying to add new client" << endl;
// }

Client *getRandomClient()
{
    if (clients.empty())
    {
        return nullptr; // Return nullptr if the map is empty
    }

    // Filter out clients with the name "Main client"
    std::vector<Client *> filteredClients;
    for (const auto &pair : clients)
    {
        if (pair.second->name != "Main Client")
        {
            filteredClients.push_back(pair.second);
        }
    }

    if (filteredClients.empty())
    {
        return nullptr; // Return nullptr if no clients are available after filtering
    }

    // Create a random number generator
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, filteredClients.size() - 1);

    // Generate a random index
    int randomIndex = dis(gen);

    return filteredClients[randomIndex];
}

void handleNewConnection(int clientSock, struct sockaddr_in &client)
{
    // Get the client's IP address and port
    char clientIp[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client.sin_addr), clientIp, INET_ADDRSTRLEN);
    int clientPort = ntohs(client.sin_port);

    std::cout << "New connection accepted: " << clientSock << " from " << clientIp << ":" << clientPort << std::endl;

    // Add new client to clients map and pollfds vector
    clients[clientSock] = new Client(clientSock);
    clients[clientSock]->ip_address = clientIp;
    clients[clientSock]->port = clientPort;
    struct pollfd newClientPollFD;
    newClientPollFD.fd = clientSock;
    newClientPollFD.events = POLLIN | POLLOUT;
    pollfds.push_back(newClientPollFD);

    std::cout << "Added new client: " << clients[clientSock]->name << " at " << clients[clientSock]->ip_address << " : " << clients[clientSock]->port << std::endl;
}

void handleDirectConnection(std::string name, std::string ip, int port)
{
    int sockfd = connectToServer(port, ip);
    if (sockfd == -1)
    {
        return;
    }

    addNewClient(sockfd, name, ip, port, false);
    // send HELO to instructor server
    std::cout << "Sending HELO,A5_42 to " << std::endl;
    sendMessage(*clients[sockfd], "HELO,A5_42");
    logMessage("|| HELO,A5_42 || sent to " + name, "", true);
    clients[sockfd]->heloSent = true;
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
    else if (tokens[0] == "LISTSERVERS" && main_client == clientSocket)
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
    else if (tokens[0] == "GETMSG" && main_client == clientSocket)
    {
        handleGetMsgCommand(clientSocket, tokens);
    }
    else if (tokens[0] == "CONNECT" && main_client == clientSocket)
    {
        handleDirectConnection(tokens[1], tokens[2], std::stoi(tokens[3]));
    }
    else
    {
        handleInvalidCommand(clientSocket, "");
        clients[clientSocket]->misbehaveCounter++;
    }
}

void clientCommand(int clientSocket, std::string buffer)
{
    handleInvalidCommand(clientSocket, buffer);
    logMessage(buffer + "|| from " + clients[clientSocket]->name + " at " + clients[clientSocket]->ip_address + " : " + std::to_string(clients[clientSocket]->port), "recieved.log", false);

    std::vector<std::vector<std::string>> all_tokens = checkMessageContentAndProcess(buffer);

    for (auto &tokens : all_tokens)
    {
        dispatchCommand(clientSocket, tokens);
    }
}

void connectToClient(Client *client)
{
    // Return if the port is not in the range of 4000-4200 or 5000 to 5005
    if ((client->port < 4000 || client->port > 4200) && (client->port < 5000 || client->port > 5005))
    {
        return;
    }
    if (!valid_id(client->name, clients))
    {
        return;
    }
    std::cout << "Connecting to " << client->name << " at " << client->ip_address << " : " << client->port << std::endl;

    int sockfd = connectToServer(client->port, client->ip_address);
    if (sockfd == -1)
    {
        return;
    }

    addNewClient(sockfd, client->name, client->ip_address, client->port, false);

    // Wait for the connection to be established
    fd_set writefds;
    FD_ZERO(&writefds);
    FD_SET(sockfd, &writefds);

    struct timeval timeout;
    timeout.tv_sec = 5; // 5 seconds timeout
    timeout.tv_usec = 0;

    int result = select(sockfd + 1, NULL, &writefds, NULL, &timeout);
    if (result <= 0)
    {
        // Timeout or error
        perror("Connection timed out or failed");
        close(sockfd);
        return;
    }

    // Check for errors
    int so_error;
    socklen_t len = sizeof(so_error);
    getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &so_error, &len);
    if (so_error != 0)
    {
        perror("Socket error");
        close(sockfd);
        return;
    }

    // Send HELO to the server
    sendMessage(*clients[sockfd], "HELO,A5_42");
    logMessage("|| HELO,A5_42 || sent to " + client->name + " at " + client->ip_address + " : " + std::to_string(client->port), "", true);
    clients[sockfd]->heloSent = true;

    // Optionally, you can wait for a response and process it
    std::string response = receiveMessage(sockfd);
    if (!response.empty())
    {
        clientCommand(sockfd, response);
    }
}

int main(int argc, char *argv[])
{
    bool finished = false;
    int listenSock; // Socket for connections to server
    int clientSock; // Socket of connecting client
    struct sockaddr_in client;
    socklen_t clientLen;

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

    logMessage("", "", false);
    logMessage("", "recieved.log", false);
    logMessage("", "sent.log", false);
    logMessage("", "messages.log", false);
    logMessage("", "client.log", false);
    // Add listening socket to the pollfds vector
    struct pollfd listenPollFD;
    listenPollFD.fd = listenSock;
    listenPollFD.events = POLLIN;
    pollfds.push_back(listenPollFD);

    // Setup all necessary variables
    time_t currentTime;
    time_t lastKeepaliveTime = time(0);
    const int keepaliveInterval = 60; // Send keepalive every minute

    while (!finished)
    {
        // Establish minimum connections to begin server
        while (clients.size() < MINSERVERS)
        {
            while (pollfds.size() == 1) // Only the listening socket is present
            {
                // Connect to instructor server until it works
                int firstSock = -1;
                while (firstSock == -1)
                {
                    firstSock = connectToServer(5001, "130.208.246.249");
                    if (firstSock == -1)
                    {
                        std::cout << "Failed to connect to Instr_1, retrying in 1 second" << std::endl;
                        sleep(10); // Wait for 1 second before retrying
                    }
                }
                addNewClient(firstSock, "Instr_1", "130.208.246.249", 5001, false);
                // Send HELO to instructor server
                sendMessage(*clients[firstSock], "HELO,A5_42");
                logMessage("|| HELO,A5_42 || sent to Instr_1 at 130.208.246.249 : 5001", "", true);
                clients[firstSock]->heloSent = true;
                // Receive HELO from Instr_1
                string response = receiveMessage(firstSock);
                clientCommand(firstSock, response);
                // Receive SERVERS from Instr_1 if they didnt arrive in the same tcp packet
                if (clients[firstSock]->servers.empty())
                {
                    response = receiveMessage(firstSock);
                    clientCommand(firstSock, response);
                }
            }
            // Pick a random server from the list map of clients
            Client *randomClient = getRandomClient();
            if (randomClient == nullptr)
            {
                randomClient = clients.begin()->second;
            }
            for (auto server : randomClient->servers)
            {
                connectToClient(server);
            }
        }

        // Poll for incoming data
        int pollCount = poll(pollfds.data(), pollfds.size(), 2000); // 1 second timeout
        if (pollCount < 0)
        {
            perror("Poll failed");
            exit(1);
        }

        // Handle events on each socket
        for (auto &pfd : pollfds)
        {
            if (pfd.revents & POLLIN) // Incoming data
            {
                if (pfd.fd == listenSock) // on the listen socket -> new connection
                {
                    // Accept new connection
                    clientLen = sizeof(client);
                    if ((clientSock = accept(listenSock, (struct sockaddr *)&client, &clientLen)) >= 0)
                    {
                        // TODO: sometimes this is causing a segfault
                        handleNewConnection(clientSock, client);
                    }
                }
                else // on a client socket
                {
                    // Process client command
                    std::string msg = receiveMessage(pfd.fd);
                    if (!msg.empty())
                    {
                        clientCommand(pfd.fd, msg);
                    }
                }
            }
        }

        // Remove clients that have not sent a message in 5 minutes
        currentTime = time(0);
        for (auto it = clients.begin(); it != clients.end();)
        {
            if (difftime(currentTime, it->second->lastMessage) > 300) // 5 minutes
            {
                removeClient(clients, pollfds, it);
            }
            else
            {
                ++it;
            }
        }

        // Remove clients that have misbehaved 5 times
        for (auto it = clients.begin(); it != clients.end();)
        {

            if (it->second->misbehaveCounter >= MAXMISBEHAVIOUR)
            {
                removeClient(clients, pollfds, it);
            }
            else
            {
                ++it;
            }
        }

        for (auto it = clients.begin(); it != clients.end();)
        {
            // make sure not to remove the main client
            if (it->second->sock == main_client)
            {
                ++it;
                continue;
            }
            // remove clients that have connected but not communicated
            if (it->second->name.empty() && difftime(currentTime, it->second->lastMessage) > 2)
            {
                removeClient(clients, pollfds, it);
            }
            else
            {
                ++it;
            }
        }

        // Remove clients if we go over the maximum

        while (clients.size() > MAXSERVERS + 1)
        {
            // Remove a random server from the list until we have 8 servers
            std::cout << "removing a random client" << std::endl;
            Client *randomClient = getRandomClient();
            if (randomClient != nullptr)
            {
                auto it = clients.find(randomClient->sock);
                if (it != clients.end())
                {
                    removeClient(clients, pollfds, it);
                }
            }
        }

        // Send keepalive messages periodically
        // TODO: propably need to check if the client is still connected before sending so the program does not stop
        if (difftime(currentTime, lastKeepaliveTime) >= keepaliveInterval)
        {
            std::cout << "|| INFO || Sending keepalive messages" << std::endl;
            for (auto &client : clients)
            {
                // check if client is in clientmap
                if (clients.find(client.first) == clients.end())
                {
                    sendKeepalive(*client.second, messageMap[client.second->name]);
                    std::cout << "sending keepalive: " << client.second->name << std::endl;
                    sleep(0.5);
                }
            }
            lastKeepaliveTime = currentTime;
        }
    }

    // Clean up and close all connections
    std::cout << "Closing all connections" << std::endl;
    for (auto &client : clients)
    {
        close(client.first);
        delete client.second;
    }

    return 0;
}
