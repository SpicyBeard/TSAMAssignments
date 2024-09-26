#include "checksum_port.h"
#include <bitset>
#include <iomanip>

// extract the secret phrase from the buffer
string get_secret_phrase(string str)
{
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
    cout << "Solving Checksum port" << endl;
    // set up a connection to the port
    pair<int, struct sockaddr_in> connection = connect_to_port(addr, port);
    int sockfd = connection.first;
    struct sockaddr_in server_addr = connection.second;
    if (sockfd < 0)
    {
        return "";
    }

    uint32_t message = htonl(secret);

    string response = send_and_receive(sockfd, &message, sizeof(message), server_addr, 5);
    if (response == "")
    {
        close(sockfd);
        return "";
    }

    char last_six_bytes[6];
    memcpy(last_six_bytes, response.c_str() + response.length() - 6, 6);

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

    // send and receive from socket
    string secretphrase = send_and_receive(sockfd, datagram, ntohs(iph->tot_len), sin, 5);
    if (secretphrase != "")
    {
        close(sockfd);
        // extract the secret phrase and return it

        secretphrase = get_secret_phrase(secretphrase);
        return secretphrase;
    }
    // All 5 attempts have failed, return false
    close(sockfd);
    return " ";
}