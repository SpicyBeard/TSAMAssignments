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
        return tokens;
    }
    return tokens;
}

void logMessage(const std::string &msg)
{
    std::time_t now = std::time(0);
    char timeStr[100];
    strftime(timeStr, sizeof(timeStr), "%d-%m-%Y", localtime(&now));
    std::string LOGFILE = std::string(timeStr) + "_server" + ".log";
    ofstream logfile;
    logfile.open(LOGFILE, ios::out | ios::app);
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
    std::string loggedMessage = "Sending message to " + client.name + " at " + client.ip_address + ":" + std::to_string(client.port) + " : " + msg;
    char messageServer[msg.length() + 2];
    bzero(messageServer, sizeof(messageServer));
    messageServer[0] = 0x01;
    memcpy(messageServer + 1, msg.c_str(), msg.length());
    messageServer[msg.length() + 1] = 0x04;
    send(client.sock, messageServer, sizeof(messageServer), 0);
}