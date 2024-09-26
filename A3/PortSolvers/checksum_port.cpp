#include "checksum_port.h"
#include <bitset>
#include <iomanip>

// extract the secret phrase from the buffer
string get_secret_phrase(const char *buffer)
{
    string str(buffer);

    // Find the positions of the first and second quotation marks
    size_t start_pos = str.find('"');
    size_t end_pos = str.find('"', start_pos + 1);

    // Check if both quotation marks are found
    if (start_pos != string::npos && end_pos != string::npos)
    {
        // Extract the substring between the quotation marks
        return str.substr(start_pos + 1, end_pos - start_pos - 1);
    }
    // Return an empty string if quotation marks are not found
    return "";
}

// solve the checksum port
string solve_checksum_port(const string &addr, int port, uint32_t secret)
{
    // set up a connection to the port
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
    uint32_t message = htonl(secret);

    // cout << "Sending to port: " << port << " & address: " << addr << endl;
    while (attempts < max_retries)
    {
        // send a message to the port containint the signature in network byte order
        if (sendto(sockfd, &message, sizeof(message), 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        {
            cerr << "Failed to send message to IP address. (checksum)" << endl;
        }

        // clear the buffer and revieve a response
        memset(buffer, 0, sizeof(buffer));
        if (recvfrom(sockfd, buffer, sizeof(buffer), 0, NULL, NULL) >= 0)
        {
            cout << "First response when sending signature: " << buffer << endl;
            break;
        }
        // try again if failed
        ++attempts;
    }
    if (attempts == max_retries)
    {
        cout << "Failed to solve checksum port." << endl;
        close(sockfd);
        return "";
    }
    char last_six_bytes[6];
    memcpy(last_six_bytes, buffer + strlen(buffer) - 6, 6);

    // Extract the checksum (first 2 bytes) in big-endian order
    uint16_t checksum;
    checksum = (last_six_bytes[0] << 8) | (last_six_bytes[1] & 0xFF);
    checksum = ntohs(checksum);

    // Extract the source address (last 4 bytes) in big-endian order
    uint32_t source_address;
    source_address = (last_six_bytes[2] << 24) | ((last_six_bytes[3] & 0xFF) << 16) |
                     ((last_six_bytes[4] & 0xFF) << 8) | (last_six_bytes[5] & 0xFF);
    source_address = ntohl(source_address);

    // Datagram to represent the packet
    char datagram[4096], *pseudogram;

    // zero out the packet buffer
    memset(datagram, 0, 4096);

    // IP header
    struct iphdr *iph = (struct iphdr *)datagram;

    // UDP header
    struct udphdr *udph = (struct udphdr *)(datagram + sizeof(struct iphdr));
    struct sockaddr_in sin;
    struct pseudo_header psh;

    // configure socket
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    sin.sin_addr.s_addr = inet_addr(addr.c_str());

    // Fill in the IP Header
    iph->ihl = 5;
    iph->version = 4;
    iph->tos = 0;
    iph->tot_len = htons(sizeof(struct iphdr) + sizeof(struct udphdr));
    iph->id = htonl(54321);
    iph->frag_off = 0x0;
    iph->ttl = 255;
    iph->protocol = IPPROTO_UDP;
    iph->check = 0;
    iph->saddr = source_address;
    iph->daddr = sin.sin_addr.s_addr;

    // Ip checksum
    iph->check = calculate_checksum((unsigned short *)datagram, ntohs(iph->tot_len));

    // UDP header
    udph->source = htons(0);
    udph->dest = htons(port);
    udph->len = htons(sizeof(struct udphdr));
    udph->check = 0;

    // Pseudo header for the udp packet
    psh.source_address = iph->saddr;
    psh.dest_address = iph->daddr;
    psh.placeholder = 0;
    psh.protocol = IPPROTO_UDP;
    psh.udp_length = htons(sizeof(struct udphdr));

    // find the correct source port so that the checksum matches
    // checksum is in network byte order
    uint16_t finalChecksum = 1;
    uint16_t checksumPort = 0;
    int psize = sizeof(struct pseudo_header) + sizeof(struct udphdr);
    while (finalChecksum != checksum)
    {
        udph->source = htons(checksumPort);
        pseudogram = (char *)malloc(psize);
        memcpy(pseudogram, (char *)&psh, sizeof(struct pseudo_header));
        memcpy(pseudogram + sizeof(struct pseudo_header), udph, sizeof(struct udphdr));
        finalChecksum = calculate_checksum((unsigned short *)pseudogram, psize); // returns in host byte order
        free(pseudogram);
        checksumPort++;
    }
    // and set the correct source.
    udph->check = checksum;
    memset(buffer, 0, sizeof(buffer));

    attempts = 0;
    while (attempts < max_retries)
    {
        // send the packet in another packet
        if (sendto(sockfd, datagram, ntohs(iph->tot_len), 0, (struct sockaddr *)&sin, sizeof(sin)) < 0)
        {
            perror("send to failed");
        }

        // recieve the response
        if (recvfrom(sockfd, buffer, sizeof(buffer), 0, NULL, NULL) < 0)
        {
            continue;
        }
        else
        {
            close(sockfd);
            // extract the secret phrase and return it
            cout << "Checksum port solved." << endl;
            string secretphrase = get_secret_phrase(buffer);
            return secretphrase;
        }
        // try again if failed
        ++attempts;
    }

    // All 5 attempts have failed, return false
    close(sockfd);
    return " ";
}