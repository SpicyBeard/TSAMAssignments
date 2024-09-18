#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <vector>
#include <cstring>
#include <utility>
#include <iomanip>

std::pair<int, struct sockaddr_in> connect_to_port(const std::string &addr, int port)
{
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        perror("Error creating socket.");
        return std::make_pair(-1, sockaddr_in());
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, addr.c_str(), &server_addr.sin_addr) <= 0)
    {
        std::cout << "Unable to set IP address" << std::endl;
        close(sockfd);
        return std::make_pair(-1, sockaddr_in());
    }

    // Set up a timeout for the socket
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout));

    return std::make_pair(sockfd, server_addr);
}

std::string check_port(const std::string &addr, int port)
{
    std::pair<int, struct sockaddr_in> connection = connect_to_port(addr, port);
    int sockfd = connection.first;
    struct sockaddr_in server_addr = connection.second;
    if (sockfd < 0)
    {
        return "";
    }

    char buffer[1024];
    int attempts = 0;
    int max_retries = 5;

    while (attempts < max_retries)
    {
        // send a message to the port
        if (sendto(sockfd, "hello?", 7, 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        {
            std::cerr << "Failed to send message to IP address." << std::endl;
            close(sockfd);
            return "";
        }

        // Wait for a response. if there is a response, the port is open
        if (recvfrom(sockfd, buffer, sizeof(buffer), 0, NULL, NULL) >= 0)
        {
            close(sockfd);

            return buffer;
        }

        // try again if failed
        ++attempts;
    }

    // All 5 attempts have failed, return false
    close(sockfd);
    return "";
}

std::string sort_port(std::string portmsg)
{
    if (portmsg.find("Enhanced X-link Port Storage Transaction Node") != std::string::npos)
    {
        return "expstn";
    }
    else if (portmsg.find("Secure Encryption Certification Relay with Enhanced Trust") != std::string::npos)
    {
        return "secret";
    }
    else if (portmsg.find("https://en.wikipedia.org/wiki/Evil_bit") != std::string::npos)
    {
        return "dark";
    }
    else
    {
        return "other";
    }
}

std::pair<std::string, int> check_and_sort_port(const std::string &ip_addr, int port)
{
    std::string port_msg = check_port(ip_addr, port);
    std::string port_type = sort_port(port_msg);
    return std::make_pair(port_type, port);
}

std::pair<int, int> solve_secret_port(const std::string &addr, int port)
// Greetings from S.E.C.R.E.T (Secure Encryption Certification Relay with Enhanced Trust)! Here's how to access the secret port I'm safeguarding:
{
    std::pair<int, struct sockaddr_in> connection = connect_to_port(addr, port);
    int sockfd = connection.first;
    struct sockaddr_in server_addr = connection.second;
    if (sockfd < 0)
    {
        return std::make_pair(-1, -1);
    }
    char buffer[1024];
    int attempts = 0;
    int max_retries = 5;
    uint8_t group_nr = 36;
    int group_secret = 0xfa899acb;
    while (attempts < max_retries)
    {
        // Send a message to the port
        //  1. Send me your group number as a single unsigned byte.
        if (sendto(sockfd, &group_nr, sizeof(group_nr), 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        {
            std::cerr << "Failed to send message to IP address." << std::endl;
            close(sockfd);
            return std::make_pair(-1, -1);
        }

        // Wait for a response.
        int bytes_received = recvfrom(sockfd, buffer, sizeof(buffer), 0, NULL, NULL);
        if (bytes_received >= 4)
        {
            // Extract the 4-byte challenge
            //  2. I'll reply with a 4-byte challenge (in network byte order) unique to your group.
            uint32_t challenge;
            memcpy(&challenge, buffer, sizeof(challenge));

            // Convert from network byte order to host byte order
            challenge = ntohl(challenge);

            //  3. Sign this challenge using the XOR operation with your group's secret (get that from your TA).
            uint32_t signature = challenge ^ group_secret;

            //  4. Reply with a 5-byte message: the first byte is your group number, followed by the 4-byte signed challenge (in network byte order).
            uint32_t network_order_four_byte = htonl(signature);
            uint8_t message[5];
            message[0] = group_nr;
            std::memcpy(&message[1], &network_order_four_byte, sizeof(network_order_four_byte));

            // Attempt to send the signed challenge and receive the response up to 5 times
            int inner_attempts = 0;
            while (inner_attempts < max_retries)
            {
                // Send the 5-byte message
                if (sendto(sockfd, message, sizeof(message), 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
                {
                    std::cerr << "Failed to send signed challenge to IP address." << std::endl;
                    close(sockfd);
                    return std::make_pair(-1, -1);
                }
                std::memset(buffer, 0, sizeof(buffer));

                //  5. If your signature is correct, I'll grant you access to the port. Good luck!
                if (recvfrom(sockfd, buffer, sizeof(buffer), 0, NULL, NULL) >= 0)
                {
                    close(sockfd);
                    // Extract the port number from the buffer
                    std::string response(buffer);
                    size_t pos = response.find_last_of(':');
                    if (pos != std::string::npos)
                    {
                        std::string number_str = response.substr(pos + 2, 4);
                        int extracted_port = std::stoi(number_str);
                        return std::make_pair(extracted_port, signature);
                    }
                }

                // try again if failed
                ++inner_attempts;
            }
            return std::make_pair(-1, -1);
        }

        // try again if failed
        ++attempts;
    }

    // All 5 attempts have failed, return false
    close(sockfd);

    return std::make_pair(-1, -1);
}

bool solve_other_port(const std::string &addr, int port, uint32_t secret)
{
    std::pair<int, struct sockaddr_in> connection = connect_to_port(addr, port);
    int sockfd = connection.first;
    struct sockaddr_in server_addr = connection.second;
    if (sockfd < 0)
    {
        return false;
    }

    char buffer[1024];

    int attempts = 0;
    int max_retries = 5;
    uint32_t message = htonl(secret);

    while (attempts < max_retries)
    {
        // send a message to the port
        // Send me a 4-byte message containing the signature you got from S.E.C.R.E.T in the first 4 bytes (in network byte order).
        if (sendto(sockfd, &message, sizeof(message), 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        {
            std::cerr << "Failed to send message to IP address first." << std::endl;
            close(sockfd);
            return false;
        }

        std::memset(buffer, 0, sizeof(buffer));
        // Wait for a response. if there is a response, the port is open
        if (recvfrom(sockfd, buffer, sizeof(buffer), 0, NULL, NULL) < 0)
        {
            return false;
        }
        // todo: extract checksum and source address from the buffer. extract the last 6 for the info in network order.
        // int checksum;
        // std::string source_address;
        // std::string response(buffer);
        // size_t pos = response.find_last_of('');
        // if (pos != std::string::npos)
        // {
        //     std::string number_str = response.substr(pos + 2, 4);
        //     int extracted_port = std::stoi(number_str);
        // }

        // Hello group 36! To get the secret phrase, reply to this message with a UDP message where the payload is a encapsulated, valid UDP IPv4 packet, that has a valid UDP checksum of [checksum], and with the source address being [port]! (Hint: all you need is a normal UDP socket which you use to send the IPv4 and UDP headers possibly with a payload) (the last 6 bytes of this message contain this information in network order)q~=?�
        int inner_attempts = 0;
        while (inner_attempts < max_retries)
        {
            // todo: create a valid UDP IPv4 packet to send in another UDP message

            // try again if failed
            ++inner_attempts;
        }

        // try again if failed
        ++attempts;
    }

    // All 5 attempts have failed, return false
    close(sockfd);
    return false;
}

bool solve_dark_port(const std::string &addr, int port, int secret)
// todo
// The dark side of network programming is a pathway to many abilities some consider to be...unnatural. I am an evil port, I will only communicate with evil processes! (https://en.wikipedia.org/wiki/Evil_bit)
// Send us a message of 4 bytes containing the signature that you created with S.E.C.R.E.T
{

    return false;
}

bool solve_expstn_port(const std::string &addr, int port)
// todo
// Greetings! I am E.X.P.S.T.N, which stands for "Enhanced X-link Port Storage Transaction Node".
// What can I do for you?
// - If you provide me with a list of secret ports (comma-separated), I can guide you on the exact sequence of "knocks" to ensure you score full marks.
// How to use E.X.P.S.T.N?
// 1. Each "knock" must be paired with both a secret phrase and your unique S.E.C.R.E.T signature.
// 2. The correct format to send a knock: First, 4 bytes containing your S.E.C.R.E.T signature, followed by the secret phrase.
// Tip: To discover the secret ports and their associated phrases, start by solving challenges on the ports detected using your port scanner. Happy hunting!
{
    return false;
}

bool solve_secret_secret_port(const std::string &addr, int port)
// todo
{
    return false;
}

int main(int argc, char *argv[])
{
    if (argc != 6)
    {
        printf("Usage: puzzlesolver <IP address> <port1> <port2> <port3> <port4>");

        exit(0);
    }

    std::string ip_addr = argv[1];
    int port1 = atoi(argv[2]);
    int port2 = atoi(argv[3]);
    int port3 = atoi(argv[4]);
    int port4 = atoi(argv[5]);

    int secret_port, dark_port, other_port, expstn_port;

    auto port1_result = check_and_sort_port(ip_addr, port1);
    auto port2_result = check_and_sort_port(ip_addr, port2);
    auto port3_result = check_and_sort_port(ip_addr, port3);
    auto port4_result = check_and_sort_port(ip_addr, port4);

    if (port1_result.first == "secret")
        secret_port = port1_result.second;
    else if (port1_result.first == "dark")
        dark_port = port1_result.second;
    else if (port1_result.first == "expstn")
        expstn_port = port1_result.second;
    else
        other_port = port1_result.second;

    if (port2_result.first == "secret")
        secret_port = port2_result.second;
    else if (port2_result.first == "dark")
        dark_port = port2_result.second;
    else if (port2_result.first == "expstn")
        expstn_port = port2_result.second;
    else
        other_port = port2_result.second;

    if (port3_result.first == "secret")
        secret_port = port3_result.second;
    else if (port3_result.first == "dark")
        dark_port = port3_result.second;
    else if (port3_result.first == "expstn")
        expstn_port = port3_result.second;
    else
        other_port = port3_result.second;

    if (port4_result.first == "secret")
        secret_port = port4_result.second;
    else if (port4_result.first == "dark")
        dark_port = port4_result.second;
    else if (port4_result.first == "expstn")
        expstn_port = port4_result.second;
    else
        other_port = port4_result.second;

    auto secret_response = solve_secret_port(ip_addr, secret_port);
    if (secret_response.first == -1 || secret_response.second == -1)
    {
        std::cout << "Failed to solve secret port." << std::endl;
        return -1;
    }

    solve_other_port(ip_addr, other_port, secret_response.second);
    solve_dark_port(ip_addr, dark_port, secret_response.second);
    solve_expstn_port(ip_addr, expstn_port);
    solve_secret_secret_port(ip_addr, secret_response.first);
}
