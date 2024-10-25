#include "utils.h"

vector<vector<string>> checkMessageContentAndProcess(const string &input)
{
    vector<vector<string>> commands;

    size_t start = 0;
    size_t end = 0;

    while ((start = input.find(0x01, end)) != string::npos)
    {
        end = input.find(0x04, start);
        if (end == string::npos)
        {
            break; // No more complete messages
        }

        string message = input.substr(start + 1, end - start - 1); // Extract message between 0x01 and 0x04
        message.erase(0, message.find_first_not_of(" \n\r"));
        message.erase(message.find_last_not_of(" \n\r") + 1);

        istringstream stream(message);
        string token;
        vector<string> tokens;
        while (getline(stream, token, ','))
        {
            tokens.push_back(token);
        }

        // Split further on ';'
        vector<string> finalTokens;
        for (const auto &tok : tokens)
        {
            istringstream subStream(tok);
            string subToken;
            while (getline(subStream, subToken, ';'))
            {
                finalTokens.push_back(subToken);
            }
        }
        commands.push_back(finalTokens);
    }

    return commands;
}

void logMessage(const string &msg, string filename, bool printToConsole)
{
    time_t now = time(0);
    char timeStr[100];
    string logFilename = filename;
    if (logFilename == "")
    {
        strftime(timeStr, sizeof(timeStr), "%d-%m-%Y", localtime(&now));
        logFilename = string(timeStr) + "_server" + ".log";
    }

    ofstream logfile;
    logfile.open(logFilename, ios::out | ios::app);
    if (!logfile.is_open())
    {
        cerr << "Failed to open log file" << endl;
        exit(1);
    }
    else
    {

        memset(timeStr, 0, sizeof(timeStr));
        strftime(timeStr, sizeof(timeStr), "%d-%m-%Y %H:%M:%S", localtime(&now));
        logfile << timeStr << ": " << msg << endl;
        if (printToConsole)
        {
            cout << timeStr << ": " << msg << endl;
        }
        logfile.close();
    }
}

// get the source ip address and port from a socket
pair<string, int> getSourceIpandPort(int sockfd)
{
    if (sockfd < 0)
    {
        return make_pair("", -1);
    }

    // Get and print the local address and port
    struct sockaddr_in socket_addr;
    socklen_t addr_len = sizeof(socket_addr);
    if (getsockname(sockfd, (struct sockaddr *)&socket_addr, &addr_len) == 0)
    {
        return make_pair(inet_ntoa(socket_addr.sin_addr), ntohs(socket_addr.sin_port));
    }
    else
    {
        return make_pair("", -1);
    }
}

bool valid_id(string id, map<int, Client *> &clients)
{
    if (id.find("A5_") != string::npos || id.find("Instr_") != string::npos)
    {
        for (auto const &client : clients)
        {
            if (client.second->name == id)
            {
                return false;
            }
        }
        return true;
    }
    return false;
}

void sendMessage(Client client, const string &msg)
{
    cout << "Sending message to " + client.name + " at " + client.ip_address + " : " + to_string(client.port) << endl;
    logMessage(msg + " || to " + client.name + " at " + client.ip_address + " : " + to_string(client.port), "sent.log", false);
    char messageServer[msg.length() + 2];
    bzero(messageServer, sizeof(messageServer));
    messageServer[0] = 0x01;
    memcpy(messageServer + 1, msg.c_str(), msg.length());
    messageServer[msg.length() + 1] = 0x04;
    send(client.sock, messageServer, sizeof(messageServer), 0);
}

string receiveMessage(int sockfd)
{
    char buffer[5000];
    memset(buffer, 0, sizeof(buffer));
    int bytesReceived = recv(sockfd, buffer, sizeof(buffer), 0);

    if (bytesReceived <= 0)
    {
        return "";
    }
    return string(buffer, bytesReceived);
}

bool connectedClient(int sock, map<int, Client *> &clients)
{
    for (auto const &client : clients)
    {
        if (client.second->sock == sock)
        {
            return true;
        }
    }
    return false;
}

int open_socket(int portno, string ip)
{
    struct sockaddr_in server_addr;
    int sock;

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Failed to open socket");
        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(portno);

    // Convert IP address from text to binary form
    if (inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr) <= 0)
    {
        cerr << "Invalid IP address: " << ip << endl;
        close(sock);
        return -1;
    }

    // Bind the socket to the specified IP and port
    if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Failed to bind to socket");
        close(sock);
        return -1;
    }

    // Start listening on the socket for incoming connections
    if (listen(sock, 10) < 0) // 10 is the backlog for incoming connections
    {
        perror("Failed to listen on socket");
        close(sock);
        return -1;
    }

    return sock; // Return the listening socket descriptor
}

int connectToServer(int portno, const std::string &ip)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        return -1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip.c_str());
    server_addr.sin_port = htons(portno);

    // Set the socket to non-blocking mode
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

    // Start the connection attempt
    cout << "Trying to connect to " << ip << " : " << portno << endl;
    int result = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (result < 0 && errno != EINPROGRESS)
    {
        close(sockfd);
        return -1;
    }

    // Use select to wait for the connection to complete or timeout
    fd_set writefds;
    FD_ZERO(&writefds);
    FD_SET(sockfd, &writefds);

    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    result = select(sockfd + 1, NULL, &writefds, NULL, &timeout);
    if (result <= 0)
    {
        // Timeout or error
        close(sockfd);
        return -1;
    }

    // Check for errors
    int so_error;
    socklen_t len = sizeof(so_error);
    getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &so_error, &len);
    if (so_error != 0)
    {
        close(sockfd);
        return -1;
    }

    // Set the socket back to blocking mode
    fcntl(sockfd, F_SETFL, flags);

    cout << "Connected to server at " << ip << " : " << portno << endl;
    return sockfd;
}

int serverConnect(int portno, const std::string &ip)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        return -1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip.c_str());
    server_addr.sin_port = htons(portno);

    // TODO: Getting stuck here
    // add some sort of timeout?
    cout << "Trying to connect to " << ip << " : " << portno << endl;
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Socket error");
        close(sockfd);
        return -1;
    }
    logMessage("|| INFO || Connected to server at " + ip + " : " + to_string(portno), "", true);

    return sockfd;
}

void sendKeepalive(Client client, int messages)
{
    string keepalive = "KEEPALIVE," + to_string(messages);
    sendMessage(client, keepalive);
}
