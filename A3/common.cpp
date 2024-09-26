#include "common.h"

// send and recieve socket
string send_and_receive(int sockfd, const void *message, size_t message_len, struct sockaddr_in &server_addr, int max_retries)
{
    char buffer[1024];
    int attempts = 0;

    while (attempts < max_retries)
    {
        // Send a message to the port
        if (sendto(sockfd, message, message_len, 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        {
            cerr << "Failed to send message to IP address. SnR" << endl;
            perror("Error");
            ++attempts;
            continue;
        }

        // Wait for a response. If there is a response, the port is open
        memset(buffer, 0, sizeof(buffer));
        if (recvfrom(sockfd, buffer, sizeof(buffer), 0, NULL, NULL) >= 0)
        {
            return string(buffer);
        }

        // Try again if failed
        ++attempts;
    }

    return "";
}

// get the source ip address and port from a socket
pair<string, int> get_source_ip_and_port(int sockfd, struct sockaddr_in server_addr)
{
    if (sockfd < 0)
    {
        return make_pair("", -1);
    }

    // Ensure the socket is connected
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        close(sockfd);
        return make_pair("", -1);
    }

    // Get and print the local address and port
    struct sockaddr_in local_addr;
    socklen_t addr_len = sizeof(local_addr);
    if (getsockname(sockfd, (struct sockaddr *)&local_addr, &addr_len) == 0)
    {
        return make_pair(inet_ntoa(local_addr.sin_addr), ntohs(local_addr.sin_port));
    }
    else
    {
        return make_pair("", -1);
    }
}

// Connect to a port on a given IP address and return the socket file descriptor and server address
pair<int, struct sockaddr_in> connect_to_port(const string &addr, int port)
{
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        perror("Error creating socket.");
        return make_pair(-1, sockaddr_in());
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, addr.c_str(), &server_addr.sin_addr) <= 0)
    {
        cout << "Unable to set IP address" << endl;
        close(sockfd);
        return make_pair(-1, sockaddr_in());
    }

    // Set up a timeout for the socket
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout));

    return make_pair(sockfd, server_addr);
}

// Compute the checksum for a given buffer
unsigned short calculate_checksum(unsigned short *ptr, int nbytes)
{
    unsigned long sum = 0;
    while (nbytes > 1)
    {
        sum += *ptr++;
        nbytes -= 2;
    }
    if (nbytes == 1)
    {
        sum += *(unsigned char *)ptr;
    }
    while (sum >> 16)
    {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    return (unsigned short)(~sum);
}

int get_secret_port_from_buffer(const char *buffer)
{
    string response(buffer);
    size_t pos = response.find_last_of(':');
    if (pos != string::npos)
    {
        string number_str = response.substr(pos + 2, 4);
        int extracted_port = stoi(number_str);
        return extracted_port;
    }
    return -1;
}