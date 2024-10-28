#include <iostream>
#include <sstream>
#include <cstring>
#include <thread>
#include <ctime>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#define LISTSERVERS 1
#define SENDMSG 2
#define GETMSG 3
#define CONNECT 4
#define EXIT 5

#define PASSCODE "Rattatoskur"

// listen for server messages
void listenServer(int serverSocket)
{
    int nread;         // Bytes read from socket
    char buffer[5000]; // Buffer for reading input

    while (true)
    {
        memset(buffer, 0, sizeof(buffer));
        nread = read(serverSocket, buffer, sizeof(buffer));

        if (nread == 0) // Server has dropped us
        {
            printf("Over and Out\n");
            exit(0);
        }
        else if (nread > 0)
        {
            std::time_t now = std::time(0);
            char timeStr[100];
            std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", std::localtime(&now));

            std::cout << timeStr << " : " << buffer << std::endl;
        }
    }
}

void displayMenu()
{
    std::cout << "Please select one of the following options:" << std::endl;
    std::cout << "1. List Servers" << std::endl;
    std::cout << "2. Send Message" << std::endl;
    std::cout << "3. Get Message" << std::endl;
    std::cout << "4. Connect to Server" << std::endl;
    std::cout << "5. Exit" << std::endl;
}

// get input for connecting to a server
std::string connectServer()
{
    std::string group;
    std::cout << "Enter the name of the group you want to connect to: ";
    std::cin >> group;
    group = group;
    std::cout << "Enter the ip address of the server you want to connect to: ";
    std::string ip;
    std::cin >> ip;
    group = group + "," + ip;
    std::cout << "Enter the port number of the server you want to connect to: ";
    std::string port;
    std::cin >> port;
    group = group + "," + port;
    std::string msg = "CONNECT," + group;
    return msg;
}

// get user command to send a message
std::string sendMsg()
{
    std::string to;
    std::string message;
    std::cout << "Enter the user you want to send the message to: ";
    std::cin >> to;
    std::cout << "Enter the message you want to send: ";
    // get all input including spaces
    std::cin.ignore();
    std::getline(std::cin, message);
    std::string msg = "SENDMSG," + to + "," + message;
    return msg;
}

// get user command to get a message
std::string getMsg()
{
    std::string group;
    std::cout << "Enter the group you want to get the message from (note that they will be removed from the server): ";
    std::cin >> group;
    std::string msg = "GETMSG," + group;
    return msg;
}

std::string listServers()
{
    return "LISTSERVERS";
}

// send the passcode to the server to establish a connection
void sendPasscode(int serverSocket)
{
    std::string passcode = PASSCODE;
    // add 0x01 to the start of the message and 0x04 to the end
    char messageServer[5000];
    bzero(messageServer, sizeof(messageServer));
    messageServer[0] = 0x01;
    // place the buffer in the messageServer after messageServer[0]
    memcpy(messageServer + 1, passcode.c_str(), passcode.length());
    messageServer[passcode.length() + 1] = 0x04;
    send(serverSocket, messageServer, strlen(messageServer), 0);
    std::cout << "Sent passcode to server" << std::endl;
}

int main(int argc, char *argv[])
{
    struct addrinfo hints, *svr;  // Network host entry for server
    struct sockaddr_in serv_addr; // Socket address for server
    int serverSocket;             // Socket used for server
    int nwrite;                   // No. bytes written to server
    char buffer[5000];            // buffer for writing to server
    bool finished;
    int set = 1; // Toggle for setsockopt

    if (argc != 3)
    {
        printf("Usage: chat_client <ip  port>\n");
        printf("Ctrl-C to terminate\n");
        exit(0);
    }

    hints.ai_family = AF_INET; // IPv4 only addresses
    hints.ai_socktype = SOCK_STREAM;

    memset(&hints, 0, sizeof(hints));

    if (getaddrinfo(argv[1], argv[2], &hints, &svr) != 0)
    {
        perror("getaddrinfo failed: ");
        exit(0);
    }

    struct hostent *server;
    server = gethostbyname(argv[1]);

    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
          (char *)&serv_addr.sin_addr.s_addr,
          server->h_length);
    serv_addr.sin_port = htons(atoi(argv[2]));

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    // Turn on SO_REUSEADDR to allow socket to be quickly reused after
    // program exit.

    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &set, sizeof(set)) < 0)
    {
        printf("Failed to set SO_REUSEADDR for port %s\n", argv[2]);
        perror("setsockopt failed: ");
    }

    if (connect(serverSocket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        // EINPROGRESS means that the connection is still being setup. Typically this
        // only occurs with non-blocking sockets. (The serverSocket above is explicitly
        // not in non-blocking mode, so this check here is just an example of how to
        // handle this properly.)
        if (errno != EINPROGRESS)
        {
            printf("Failed to open socket to server: %s\n", argv[1]);
            perror("Connect failed: ");
            exit(0);
        }
    }

    // Listen and print replies from server
    std::thread serverThread(listenServer, serverSocket);
    sendPasscode(serverSocket);

    finished = false;
    while (!finished)
    {
        displayMenu();
        int choice;
        std::cin >> choice;
        std::string msg;

        switch (choice)
        {
        case SENDMSG:
            msg = sendMsg();
            break;
        case LISTSERVERS:
            msg = listServers();
            break;
        case GETMSG:
            msg = getMsg();
            break;
        case CONNECT:
            msg = connectServer();
            break;
        case EXIT:
            exit(0);
            break;
        }
        // get user command

        bzero(buffer, sizeof(buffer));
        memcpy(buffer, msg.c_str(), msg.length());
        // add 0x01 to the start of the message and 0x04 to the end
        char messageServer[5000];
        bzero(messageServer, sizeof(messageServer));
        messageServer[0] = 0x01;

        // place the buffer in the messageServer after messageServer[0]
        memcpy(messageServer + 1, buffer, strlen(buffer));
        messageServer[strlen(buffer) + 1] = 0x04;

        std::cout << "Sending msg: " << msg << std::endl;
        nwrite = send(serverSocket, messageServer, strlen(messageServer), 0);

        std::time_t now = std::time(0);
        char timeStr[100];
        std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
        std::cout << timeStr << " : " << buffer << std::endl;

        if (nwrite == -1)
        {
            perror("send() to server failed: ");
            finished = true;
        }

        sleep(1);
    }
}
