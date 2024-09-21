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
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netdb.h>

using namespace std;

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

string check_port(const string &addr, int port)
{
    pair<int, struct sockaddr_in> connection = connect_to_port(addr, port);
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
            cerr << "Failed to send message to IP address." << endl;
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

string sort_port(string portmsg)
{
    if (portmsg.find("Enhanced X-link Port Storage Transaction Node") != string::npos)
    {
        return "expstn";
    }
    else if (portmsg.find("Secure Encryption Certification Relay with Enhanced Trust") != string::npos)
    {
        return "secret";
    }
    else if (portmsg.find("https://en.wikipedia.org/wiki/Evil_bit") != string::npos)
    {
        return "dark";
    }
    else
    {
        return "checksum";
    }
}

pair<string, int> check_and_sort_port(const string &ip_addr, int port)
{
    string port_msg = check_port(ip_addr, port);
    string port_type = sort_port(port_msg);
    return make_pair(port_type, port);
}

pair<int, int> solve_secret_port(const string &addr, int port)
// Greetings from S.E.C.R.E.T (Secure Encryption Certification Relay with Enhanced Trust)! Here's how to access the secret port I'm safeguarding:
{
    pair<int, struct sockaddr_in> connection = connect_to_port(addr, port);
    int sockfd = connection.first;
    struct sockaddr_in server_addr = connection.second;
    if (sockfd < 0)
    {
        return make_pair(-1, -1);
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
            cerr << "Failed to send message to IP address." << endl;
            close(sockfd);
            return make_pair(-1, -1);
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
            memcpy(&message[1], &network_order_four_byte, sizeof(network_order_four_byte));

            // Attempt to send the signed challenge and receive the response up to 5 times
            int inner_attempts = 0;
            while (inner_attempts < max_retries)
            {
                // Send the 5-byte message
                if (sendto(sockfd, message, sizeof(message), 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
                {
                    cerr << "Failed to send signed challenge to IP address." << endl;
                    close(sockfd);
                    return make_pair(-1, -1);
                }
                memset(buffer, 0, sizeof(buffer));

                //  5. If your signature is correct, I'll grant you access to the port. Good luck!
                if (recvfrom(sockfd, buffer, sizeof(buffer), 0, NULL, NULL) >= 0)
                {
                    close(sockfd);
                    // Extract the port number from the buffer
                    string response(buffer);
                    size_t pos = response.find_last_of(':');
                    if (pos != string::npos)
                    {
                        string number_str = response.substr(pos + 2, 4);
                        int extracted_port = stoi(number_str);
                        return make_pair(extracted_port, signature);
                    }
                }

                // try again if failed
                ++inner_attempts;
            }
            return make_pair(-1, -1);
        }

        // try again if failed
        ++attempts;
    }

    // All 5 attempts have failed, return false
    close(sockfd);

    return make_pair(-1, -1);
}

bool solve_checksum_port(const string &addr, int port, uint32_t secret)
{
    pair<int, struct sockaddr_in> connection = connect_to_port(addr, port);
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
            cerr << "Failed to send message to IP address first." << endl;
            close(sockfd);
            return false;
        }

        memset(buffer, 0, sizeof(buffer));
        // Wait for a response. if there is a response, the port is open
        if (recvfrom(sockfd, buffer, sizeof(buffer), 0, NULL, NULL) < 0)
        {
            return false;
        }
        cout << buffer << endl;
        int checksum;
        string source_address, information;
        string response(buffer);

        size_t pos = response.find("0x");
        if (pos != string::npos)
        {
            string number_str = response.substr(pos + 2, 6);
            checksum = stoi(number_str, nullptr, 16);
        }
        size_t start_pos = response.find("being");
        start_pos += 6;
        size_t end_pos = response.find("!");
        end_pos -= 1;
        source_address = response.substr(start_pos, end_pos);

        // todo: maybe use this instead of extracting from the buffer
        information = response.substr(response.length() - 6);

        return true;
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

struct pseudo_header
{
    u_int32_t source_address;
    u_int32_t dest_address;
    u_int8_t placeholder;
    u_int8_t protocol;
    u_int16_t udp_length;
};

unsigned short csum(unsigned short *ptr, int nbytes)
{
    register long sum;
    unsigned short oddbyte;
    register short answer;

    sum = 0;
    while (nbytes > 1)
    {
        sum += *ptr++;
        nbytes -= 2;
    }
    if (nbytes == 1)
    {
        oddbyte = 0;
        *((u_char *)&oddbyte) = *(u_char *)ptr;
        sum += oddbyte;
    }

    sum = (sum >> 16) + (sum & 0xffff);
    sum = sum + (sum >> 16);
    answer = (short)~sum;

    return (answer);
}

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

int solve_dark_port(const string &addr, int port, int secret)
// todo
// The dark side of network programming is a pathway to many abilities some consider to be...unnatural. I am an evil port, I will only communicate with evil processes! (https://en.wikipedia.org/wiki/Evil_bit)
// Send us a message of 4 bytes containing the signature that you created with S.E.C.R.E.T
{
    pair<int, struct sockaddr_in> connection = connect_to_port(addr, port);
    int udp_socket = connection.first;
    struct sockaddr_in server_addr = connection.second;
    pair<string, int> source_ip_and_port = get_source_ip_and_port(udp_socket, server_addr);
    string source_ip = source_ip_and_port.first;
    int source_port = source_ip_and_port.second;
    int s = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    if (s < 0)
    {
        perror("Error creating socket.");
        return -1;
    }
    // Set socket option to include IP headers
    int one = 1;
    if (setsockopt(s, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one)) < 0)
    {
        perror("Failed to set socket option");
        close(s);
        return -1;
    }

    // Set a timeout for the socket
    struct timeval timeout;
    timeout.tv_sec = 1; // 1 second timeout
    timeout.tv_usec = 0;
    if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
    {
        perror("Failed to set socket receive timeout");
        close(s);
        return -1;
    }

    // Datagram to represent the packet
    char datagram[4096], *data, *pseudogram;

    // zero out the packet buffer
    memset(datagram, 0, 4096);

    // IP header
    struct iphdr *iph = (struct iphdr *)datagram;

    // UDP header
    struct udphdr *udph = (struct udphdr *)(datagram + sizeof(struct iphdr));
    struct sockaddr_in sin;
    struct pseudo_header psh;

    // Data part
    data = datagram + sizeof(struct iphdr) + sizeof(struct udphdr);
    uint32_t secret_network_order = htonl(secret);
    memcpy(data, &secret_network_order, sizeof(secret_network_order));

    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    sin.sin_addr.s_addr = inet_addr(addr.c_str());

    // Fill in the IP Header
    iph->ihl = 5;
    iph->version = 4;
    iph->tos = 0;
    iph->tot_len = sizeof(struct iphdr) + sizeof(struct udphdr) + sizeof(secret_network_order);
    iph->id = htonl(54321); // Id of this packet
    iph->frag_off = 0x80;   // evil bit?
    iph->ttl = 255;
    iph->protocol = IPPROTO_UDP;
    iph->check = 0; // Set to 0 before calculating checksum
    iph->saddr = inet_addr(source_ip.c_str());
    iph->daddr = sin.sin_addr.s_addr;

    // Ip checksum
    iph->check = csum((unsigned short *)datagram, iph->tot_len);

    // UDP header
    udph->source = htons(source_port); // Let the OS assign the source port dynamically
    udph->dest = htons(port);
    udph->len = htons(sizeof(struct udphdr) + sizeof(secret_network_order));
    udph->check = 0; // leave checksum 0 now, filled later by pseudo header

    // Now the UDP checksum using the pseudo header
    psh.source_address = inet_addr(source_ip.c_str());
    psh.dest_address = sin.sin_addr.s_addr;
    psh.placeholder = 0;
    psh.protocol = IPPROTO_UDP;
    psh.udp_length = htons(sizeof(struct udphdr) + sizeof(secret_network_order));

    int psize = sizeof(struct pseudo_header) + sizeof(struct udphdr) + sizeof(secret_network_order);
    pseudogram = (char *)malloc(psize);

    memcpy(pseudogram, (char *)&psh, sizeof(struct pseudo_header));
    memcpy(pseudogram + sizeof(struct pseudo_header), udph, sizeof(struct udphdr) + sizeof(secret_network_order));

    udph->check = csum((unsigned short *)pseudogram, psize);

    int attempts = 0;
    int max_retries = 5;
    // string messages

    while (attempts < max_retries)
    {
        if (sendto(s, datagram, iph->tot_len, 0, (struct sockaddr *)&sin, sizeof(sin)) < 0)
        {
            perror("sendto failed");
        }
        // Data send successfully
        else
        {
            char buffer[1024];
            memset(buffer, 0, sizeof(buffer));

            // Check if a response is received
            if (recvfrom(udp_socket, buffer, sizeof(buffer), 0, NULL, NULL) >= 0)
            {
                close(udp_socket);
                close(s);
                string response(buffer);
                size_t pos = response.find_last_of(':');
                if (pos != string::npos)
                {
                    string number_str = response.substr(pos + 2, 5);
                    int extracted_port = stoi(number_str);
                    return extracted_port;
                }
            }
            else
            {
                cout << "No response received." << endl;
            }
        }
        attempts++;
    }

    close(udp_socket);
    close(s);
    return -1;
}

bool solve_expstn_port(const string &addr, int port)
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

bool solve_secret_secret_port(const string &addr, int port, string ports)
// todo
{
    return false;
    // cout << ports << endl;
    // pair<int, struct sockaddr_in> connection = connect_to_port(addr, port);
    // int sockfd = connection.first;
    // struct sockaddr_in server_addr = connection.second;
    // if (sockfd < 0)
    // {
    //     return false;
    // }

    // char buffer[1024];
    // int attempts = 0;
    // int max_retries = 5;

    // while (attempts < max_retries)
    // {
    //     // send a message to the port
    //     if (sendto(sockfd, "hi", 3, 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    //     {
    //         cerr << "Failed to send message to IP address." << endl;
    //         close(sockfd);
    //         return "";
    //     }

    //     // Wait for a response. if there is a response, the port is open
    //     if (recvfrom(sockfd, buffer, sizeof(buffer), 0, NULL, NULL) >= 0)
    //     {
    //         close(sockfd);
    //         cout << buffer << endl;
    //         return true;
    //     }

    //     // try again if failed
    //     ++attempts;
    // }

    // // All 5 attempts have failed, return false
    // close(sockfd);
    // return false;
}

int main(int argc, char *argv[])
{
    if (argc != 6)
    {
        printf("Usage: puzzlesolver <IP address> <port1> <port2> <port3> <port4>");

        exit(0);
    }

    string ip_addr = argv[1];
    int port1 = atoi(argv[2]);
    int port2 = atoi(argv[3]);
    int port3 = atoi(argv[4]);
    int port4 = atoi(argv[5]);

    int secret_port, dark_port, checksum_port, expstn_port;
    string ports;
    ports = to_string(port1) + "," + to_string(port2) + "," + to_string(port3) + "," + to_string(port4);

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
        checksum_port = port1_result.second;

    if (port2_result.first == "secret")
        secret_port = port2_result.second;
    else if (port2_result.first == "dark")
        dark_port = port2_result.second;
    else if (port2_result.first == "expstn")
        expstn_port = port2_result.second;
    else
        checksum_port = port2_result.second;

    if (port3_result.first == "secret")
        secret_port = port3_result.second;
    else if (port3_result.first == "dark")
        dark_port = port3_result.second;
    else if (port3_result.first == "expstn")
        expstn_port = port3_result.second;
    else
        checksum_port = port3_result.second;

    if (port4_result.first == "secret")
        secret_port = port4_result.second;
    else if (port4_result.first == "dark")
        dark_port = port4_result.second;
    else if (port4_result.first == "expstn")
        expstn_port = port4_result.second;
    else
        checksum_port = port4_result.second;

    auto secret_response = solve_secret_port(ip_addr, secret_port);
    if (secret_response.first == -1 || secret_response.second == -1)
    {
        cout << "Failed to solve secret port." << endl;
        return -1;
    }

    solve_secret_secret_port(ip_addr, secret_response.first, ports);
    // solve_checksum_port(ip_addr, checksum_port, secret_response.second);
    solve_dark_port(ip_addr, dark_port, secret_response.second);
    // solve_expstn_port(ip_addr, expstn_port);
}
