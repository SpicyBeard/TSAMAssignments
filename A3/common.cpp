#include "common.h"

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