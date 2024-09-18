#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <vector>
#include <mutex>
#include <thread>

std::mutex mutex;

bool port_is_open(const std::string &addr, int port)
{
    // set up the socket file descriptor
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        perror("Error creating socket.");
        return false;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, addr.c_str(), &server_addr.sin_addr) <= 0)
    {
        std::cout << "Unable to set ip address" << std::endl;
        close(sockfd);
        return false;
    }

    // set up a timeout for each port
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout));

    char buffer[1];
    int attempts = 0;
    int max_retries = 2;

    while (attempts < max_retries)
    {
        // send a message to the port
        if (sendto(sockfd, "hello?", 7, 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        {
            std::cerr << "Failed to send message to IP address." << std::endl;
            close(sockfd);
            return false;
        }

        // Wait for a response. if there is a response, the port is open
        if (recvfrom(sockfd, buffer, sizeof(buffer), 0, NULL, NULL) >= 0)
        {
            close(sockfd);
            return true;
        }

        // try again if failed
        ++attempts;
    }

    // If we reach here, all attempts have failed
    close(sockfd);
    return false;
}

// check if a port is open and add it to the open_ports vector
void check_port(const std::string &address, int port, std::vector<int> &open_ports)
{
    if (port_is_open(address, port))
    {
        // if the port is open, lock the mutex and add it to the open_ports vector
        std::lock_guard<std::mutex> lock(mutex);
        open_ports.push_back(port);
    }
}

int main(int argc, char *argv[])
{
    // if the incorrect number of arguments are passed, print usage and exit
    if (argc != 4)
    {
        printf("Usage: scanner <IP address> <low-port> <high-port>");
        exit(0);
    }

    // set up all necessary variables
    std::string ip_addr = argv[1];
    int low_port = atoi(argv[2]);
    int high_port = atoi(argv[3]);
    // a vector to store all open ports and another to store all threads
    std::vector<int> open_ports;
    std::vector<std::thread> threads;

    // create thread for each port and check if it is open
    for (int port = low_port; port <= high_port; ++port)
    {
        threads.emplace_back(check_port, ip_addr, port, std::ref(open_ports));
    }

    // Wait for all threads to finish and join them
    for (std::thread &t : threads)
    {
        if (t.joinable())
        {
            t.join();
        }
    }

    // prints all open ports, or if none are open
    if (open_ports.size() == 0)
    {
        std::cout << "No open ports found." << std::endl;
    }
    else
    {
        std::cout << "Open ports: ";
        for (int port : open_ports)
        {
            std::cout << port << " ";
        }
        std::cout << std::endl;
    }

    return 0;
}