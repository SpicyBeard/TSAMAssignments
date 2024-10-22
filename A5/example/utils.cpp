#include "utils.h"

vector<string> checkMessageContentAndProcess(char *buffer)
{

    vector<string> tokens;
    if (buffer[0] == 0x01 && buffer[strlen(buffer) - 1] == 0x04)
    {
        string input(buffer);

        input.erase(0, 1); // Remove starting 0x01 marker

        input.erase(input.length() - 1); // Remove ending 0x04 marker

        // Trim any extra whitespaces, newline, etc.
        input.erase(0, input.find_first_not_of(" \n\r"));
        input.erase(input.find_last_not_of(" \n\r") + 1);
        string token;
        istringstream stream(input);

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
        tokens = finalTokens;
        return tokens;
    }
    return tokens;
}

void logMessage(const std::string &msg, std::string filename = "")
{
    std::time_t now = std::time(0);
    char timeStr[100];
    if (filename == "")
    {
        strftime(timeStr, sizeof(timeStr), "%d-%m-%Y", localtime(&now));
        std::string filename = std::string(timeStr) + "_server" + ".log";
    }

    ofstream logfile;
    logfile.open(filename, ios::out | ios::app);
    if (!logfile.is_open())
    {
        cerr << "Failed to open log file" << endl;
        exit(1);
    }
    else
    {

        memcpy(timeStr, "", sizeof(timeStr));
        strftime(timeStr, sizeof(timeStr), "%d-%m-%Y %H:%M:%S", localtime(&now));
        logfile << timeStr << ": " << msg << endl;
        cout << timeStr << ": " << msg << endl;
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
    if (id.find("A5_") != string::npos || id.find("Inst_") != string::npos)
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

void sendMessage(Client client, const std::string &msg)
{
    printf("Sending message to %s at %s:%d\n", client.name.c_str(), client.ip_address.c_str(), client.port);
    char messageServer[msg.length() + 2];
    bzero(messageServer, sizeof(messageServer));
    messageServer[0] = 0x01;
    memcpy(messageServer + 1, msg.c_str(), msg.length());
    messageServer[msg.length() + 1] = 0x04;
    send(client.sock, messageServer, sizeof(messageServer), 0);
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

// int open_socket(int portno, string ip)
// {
//     struct sockaddr_in sk_addr; // address settings for bind()
//     int sock;                   // socket opened for this port
//     int set = 1;                // for setsockopt

//     // Create socket for connection. Set to be non-blocking, so recv will
//     // return immediately if there isn't anything waiting to be read.
// #ifdef __APPLE__
//     if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
//     {
//         perror("Failed to open socket");
//         return (-1);
//     }
// #else
//     if ((sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) < 0)
//     {
//         perror("Failed to open socket");
//         return (-1);
//     }
// #endif

//     // Turn on SO_REUSEADDR to allow socket to be quickly reused after
//     // program exit.

//     if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &set, sizeof(set)) < 0)
//     {
//         perror("Failed to set SO_REUSEADDR:");
//     }
//     set = 1;
// #ifdef __APPLE__
//     if (setsockopt(sock, SOL_SOCKET, SOCK_NONBLOCK, &set, sizeof(set)) < 0)
//     {
//         perror("Failed to set SOCK_NOBBLOCK");
//     }
// #endif

//     memset(&sk_addr, 0, sizeof(sk_addr));

//     sk_addr.sin_family = AF_INET;
//     sk_addr.sin_port = htons(portno);
//     if (inet_pton(AF_INET, ip.c_str(), &sk_addr.sin_addr) <= 0)
//     {
//         cout << "Unable to set IP address" << endl;
//         return -1;
//     }

//     // Bind to socket to listen for connections from clients

//     if (bind(sock, (struct sockaddr *)&sk_addr, sizeof(sk_addr)) < 0)
//     {
//         perror("Failed to bind to socket:");
//         return (-1);
//     }
//     else
//     {
//         return (sock);
//     }
// }

int open_socket(int portno, std::string ip)
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
        std::cerr << "Invalid IP address: " << ip << std::endl;
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

int connect_to_server(int portno, const std::string &ip)
{
    struct sockaddr_in server_addr;
    int sock;

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Failed to create socket");
        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(portno);

    // Convert IP address from text to binary form
    if (inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr) <= 0)
    {
        std::cerr << "Invalid IP address: " << ip << std::endl;
        close(sock);
        return -1;
    }

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Connection Failed");
        close(sock);
        return -1;
    }

    return sock; // Return the socket descriptor for the established connection
}