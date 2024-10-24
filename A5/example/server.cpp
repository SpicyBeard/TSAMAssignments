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

// Process command from client on the server

void clientCommand(int clientSocket, const std::string buffer)
{
    if (buffer.empty())
    {
        std::string msg = "Invalid command from " + clients[clientSocket]->name + " at " + clients[clientSocket]->ip_address + " : " + std::to_string(clients[clientSocket]->port);
        logMessage(msg, "");
        clients[clientSocket]->misbehaveCounter++;
        return;
    }
    std::vector<std::vector<std::string>> all_tokens = checkMessageContentAndProcess(buffer);

    for (auto tokens : all_tokens)
    {
        if (tokens[0].compare("Rattatoskur") == 0)
        {
            std::string msg = "Main Client connected to Server";
            logMessage(msg, "");
            send(clientSocket, msg.c_str(), msg.length(), 0);
            main_client = clientSocket;
        }
        else if (tokens[0].compare("HELO") == 0 && tokens.size() == 2)
        {
            logMessage("|| HELO," + tokens[1] + " || recieved from " + tokens[1] + " at " + clients[clientSocket]->ip_address + " : " + std::to_string(clients[clientSocket]->port), "");
            clients[clientSocket]->lastMessage = time(0);
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
                    sendMessage(*clients[clientSocket], "HELO,A5_42");
                    logMessage("|| HELO,A5_42 || sent to " + clients[clientSocket]->name + " at " + clients[clientSocket]->ip_address + " : " + std::to_string(clients[clientSocket]->port), "");
                }
                sendMessage(*clients[clientSocket], response);
                logMessage("|| SERVERS || sent to " + clients[clientSocket]->name + " at " + clients[clientSocket]->ip_address + " : " + std::to_string(clients[clientSocket]->port), "");
                // reply with SERVERS
            }
        }
        else if (tokens[0].compare("SERVERS") == 0)
        {
            if (tokens.size() < 2)
            {
                return;
            }
            std::string loggedMessage = "|| SERVERS || received from " + clients[clientSocket]->name + " at " + clients[clientSocket]->ip_address + " : " + std::to_string(clients[clientSocket]->port);
            logMessage(loggedMessage, "");
            clients[clientSocket]->lastMessage = time(0);
            // for (int i = 1; i < 4; i++)
            // {
            //     if (clients[clientSocket]->name != tokens[i] && clients[clientSocket]->ip_address != tokens[i + 1] && clients[clientSocket]->port != std::stoi(tokens[i + 2]))
            //     {
            //         logMessage("|| ERROR || " + clients[clientSocket]->name + " at " + clients[clientSocket]->ip_address + " : " + std::to_string(clients[clientSocket]->port), "");
            //         logMessage("\tServer did not send themselves as the first server", "");
            //         clients[clientSocket]->misbehaveCounter++;
            //         return;
            //     }
            // }
            for (size_t i = 4; i < tokens.size(); i += 3)
            {
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
        else if (tokens[0].compare("GETMSGS") == 0 && tokens.size() == 2 && connectedClient(clientSocket, clients))
        {
            // GETMSGS,<GROUP ID>
            // check if groupid is valid
            std::string msg = "|| GETMSGS || received from group number " + tokens[1] + "requested by: " + clients[clientSocket]->name + " at " + clients[clientSocket]->ip_address + " : " + std::to_string(clients[clientSocket]->port);
            logMessage(msg, "");
            clients[clientSocket]->lastMessage = time(0);
            for (auto message : messageVector)
            {
                if (message.to.compare(tokens[1]) == 0)
                {
                    std::string response = "SENDMSG," + message.to + "," + message.from + "," + message.message;
                    sendMessage(*clients[clientSocket], response);
                    logMessage("|| SENDMSG || sent to " + clients[clientSocket]->name + " at " + clients[clientSocket]->ip_address + " : " + std::to_string(clients[clientSocket]->port) + " from group " + message.from, "");
                }
            }
        }
        else if (tokens[0].compare("SENDMSG") == 0 && tokens.size() == 4 && connectedClient(clientSocket, clients))
        {

            logMessage("|| SENDMSG || received from " + tokens[2] + " to group number " + tokens[1], "");
            clients[clientSocket]->lastMessage = time(0);
            if (tokens[1] == "A5_42")
            {
                // also add to messageVector, we will use GETMSGS from client to get them
                logMessage("Received message from " + tokens[2] + " Message Content:" + tokens[3], "Message_log.txt");
            }
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
        else if (tokens[0].compare("STATUSREQ") == 0 && connectedClient(clientSocket, clients))
        {
            // reply with STATUSRESP
            logMessage("|| STATUSREQ || received from " + clients[clientSocket]->name + " at " + clients[clientSocket]->ip_address + " : " + std::to_string(clients[clientSocket]->port), "");
            clients[clientSocket]->lastMessage = time(0);
            std::string response = "STATUSRESP,";
            for (auto message : messageMap)
            {
                response += message.first + "," + std::to_string(message.second) + ",";
            }
            sendMessage(*clients[clientSocket], response);
            logMessage("|| STATUSRESP || sent to " + clients[clientSocket]->name + " at " + clients[clientSocket]->ip_address + " : " + std::to_string(clients[clientSocket]->port), "");
        }
        else if (tokens[0].compare("STATUSRESP") == 0 && tokens.size() >= 1 && connectedClient(clientSocket, clients))
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
                    for (auto client : clients)
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
            logMessage(msg, "client.log");
            send(clientSocket, msg.c_str(), msg.length(), 0);
        }
        else if (tokens[0].compare("GETMSG") == 0 && tokens.size() == 2 && main_client == clientSocket)
        {
            std::string msg = "Getting message from group number " + tokens[1];
            logMessage(msg, "client.log");
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
            logMessage(msg, "client.log");
            send(clientSocket, msg.c_str(), msg.length(), 0);
        }
        else
        {
            logMessage("|| ERROR || " + clients[clientSocket]->name + " at " + clients[clientSocket]->ip_address + " : " + std::to_string(clients[clientSocket]->port), "");
            logMessage("Invalid command : " + tokens[0], "");
            std::string msg = "Invalid command from " + clients[clientSocket]->name + " at " + clients[clientSocket]->ip_address + " : " + std::to_string(clients[clientSocket]->port);
            logMessage(msg, "");
            clients[clientSocket]->misbehaveCounter++;
            return;
        }
    }
}

// Helper function to remove client from poll list
void removeClientFromPoll(int clientSocket)
{
    auto it = std::remove_if(pollfds.begin(), pollfds.end(), [clientSocket](struct pollfd &pfd)
                             { return pfd.fd == clientSocket; });
    pollfds.erase(it, pollfds.end());
}

void closeClient(int clientSocket, std::vector<struct pollfd> &pollfds)
{
    logMessage("|| INFO || Closing connection to " + clients[clientSocket]->name + " at " + clients[clientSocket]->ip_address + " : " + std::to_string(clients[clientSocket]->port), "");
    close(clientSocket);

    // Remove the socket from the pollfds vector
    auto it = std::remove_if(pollfds.begin(), pollfds.end(),
                             [clientSocket](struct pollfd &pfd)
                             {
                                 return pfd.fd == clientSocket;
                             });
    pollfds.erase(it, pollfds.end());
    // remove client from clients map
    clients.erase(clientSocket);
}

void addNewClient(int clientSocket, string name, string ip, int port, bool heloSent)
{
    // Create a new client entry in the clients map and add to pollfds
    cout << "Adding new client: " + name + " at " + ip + " : " + std::to_string(port) << endl;
    clients[clientSocket] = new Client(clientSocket);
    clients[clientSocket]->name = name;
    clients[clientSocket]->ip_address = ip;
    clients[clientSocket]->port = port;
    clients[clientSocket]->heloSent = heloSent;
    clients[clientSocket]->misbehaveCounter = 0;
    clients[clientSocket]->servers = {};
    // Add new client to the pollfds vector
    struct pollfd newClientPollFD;
    newClientPollFD.fd = clientSocket;
    newClientPollFD.events = POLLIN | POLLOUT;
    pollfds.push_back(newClientPollFD);
}

void connectToClient(Client *client)
{

    // return if the port is not in the range of 4000-4200 or 5000 to 5005
    if ((client->port < 4000 || client->port > 4200) && (client->port < 5000 || client->port > 5005))
    {
        return;
    }
    if (!valid_id(client->name, clients))
    {
        return;
    }
    cout << "Connecting to " << client->name << " at " << client->ip_address << " : " << client->port << endl;

    int sockfd = connectToServer(client->port, client->ip_address);
    if (sockfd == -1)
    {
        return;
    }

    // send HELO to instructor server
    cout << "Sending HELO,A5_42 to client " + client->name + " at " + client->ip_address + " : " + std::to_string(client->port) << endl;
    sendMessage(*client, "HELO,A5_42");
    logMessage("|| HELO,A5_42 || sent to " + client->name + " at " + client->ip_address + " : " + std::to_string(client->port), "");
    // string response = receiveMessage(sockfd);
    // if (response.length() > 0)
    // {
    //     cout << "Received message from " << client->name << endl;
    //     cout << response << endl;
    //     clientCommand(sockfd, response);
    // }
    // cout << "Trying to add new client" << endl;
    addNewClient(sockfd, client->name, client->ip_address, client->port, true);
}

Client *getRandomClient()
{
    if (clients.empty())
    {
        return nullptr; // Return nullptr if the map is empty
    }

    // Create a random number generator
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, clients.size() - 1);

    // Generate a random index
    int randomIndex = dis(gen);

    // Advance the iterator to the random index
    auto it = clients.begin();
    std::advance(it, randomIndex);

    return it->second; // Return the randomly chosen client
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

    // Setup all necessary variables
    time_t currentTime;
    time_t lastKeepaliveTime = time(0);
    const int keepaliveInterval = 60; // Send keepalive every minute

    while (!finished)
    {
        // Establish minimum connections to begin server
        while (pollfds.size() < MINSERVERS)
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
                        cout << "Failed to connect to Instr_1, retrying in 1 second" << endl;
                        sleep(1); // Wait for 1 second before retrying
                    }
                }
                addNewClient(firstSock, "Instr_1", "130.208.246.249", 5001, false);
                // Send HELO to instructor server
                sendMessage(*clients[firstSock], "HELO,A5_42");
                logMessage("|| HELO,A5_42 || sent to Instr_1 at 130.208.246.249 : 5001", "");
                clients[firstSock]->heloSent = true;
                // Receive HELO from Instr_1
                string response = receiveMessage(firstSock);
                clientCommand(firstSock, response);
                // Receive SERVERS from Instr_1

                response = receiveMessage(firstSock);
                clientCommand(firstSock, response);
            }
            // Pick a random server from the list map of clients
            Client *randomClient = getRandomClient();
            for (auto server : randomClient->servers)
            {
                connectToClient(server);
            }
        }

        // Poll for incoming data
        int pollCount = poll(pollfds.data(), pollfds.size(), 1000); // 1 second timeout
        if (pollCount < 0)
        {
            perror("Poll failed");
            exit(1);
        }

        // Handle events on each socket
        for (auto &pfd : pollfds)
        {
            if (pfd.revents & POLLIN)
            {
                if (pfd.fd == listenSock)
                {
                    // Accept new connection
                    clientLen = sizeof(client);
                    clientSock = accept(listenSock, (struct sockaddr *)&client, &clientLen);
                    if (clientSock < 0)
                    {
                        perror("Accept failed");
                        continue;
                    }
                    // TODO: fix this, the port is not correct
                    cout << "New connection accepted: " + std::to_string(clientSock) << endl;
                    // Add new client to clients map and pollfds vector
                    std::pair<std::string, int> source= getSourceIpandPort(clientSock);
                    clients[clientSock] = new Client(clientSock);
                    clients[clientSock]->ip_address = source.first;
                    clients[clientSock]->port = source.second;
                    struct pollfd newClientPollFD;
                    newClientPollFD.fd = clientSock;
                    newClientPollFD.events = POLLIN | POLLOUT;
                    pollfds.push_back(newClientPollFD);
                    cout << "Added new client: " + clients[clientSock]->name + " at " + clients[clientSock]->ip_address + " : " + std::to_string(clients[clientSock]->port) << endl;
                }
                else
                {
                    // Handle incoming message from client
                    int bytesRecieved = recv(pfd.fd, buffer, sizeof(buffer), 0);
                    if (bytesRecieved <= 0)
                    {
                        // Close client connection if error or disconnect
                        closeClient(pfd.fd, pollfds);
                    }
                    else
                    {
                        // Process client command
                        clientCommand(pfd.fd, buffer);
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
                closeClient(it->first, pollfds);
                delete it->second;
                it = clients.erase(it);
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
                // TODO: causes segfault
                logMessage("Removing misbehaving client: " + it->second->name, "");
                closeClient(it->first, pollfds);
                delete it->second;
                it = clients.erase(it);
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
            cout << "removing a random client" << endl;
            Client *randomClient = getRandomClient();
            closeClient(randomClient->sock, pollfds);
        }

        // Send keepalive messages periodically
        if (difftime(currentTime, lastKeepaliveTime) >= keepaliveInterval)
        {
            cout << "|| INFO || Sending keepalive messages" << endl;
            for (auto &client : clients)
            {
                sendKeepalive(*client.second, messageMap[client.second->name]);
            }
            lastKeepaliveTime = currentTime;
        }
    }

    // Clean up and close all connections
    cout << "Closing all connections" << endl;
    for (auto &client : clients)
    {
        close(client.first);
        delete client.second;
    }

    return 0;
}
