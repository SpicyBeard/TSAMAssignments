#include "evil_port.h"

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

    // Get and return the local address and port
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

int solve_evil_port(const string &addr, int port, int secret)
// The dark side of network programming is a pathway to many abilities some consider to be...unnatural. I am an evil port, I will only communicate with evil processes! (https://en.wikipedia.org/wiki/Evil_bit)
// Send us a message of 4 bytes containing the signature that you created with S.E.C.R.E.T
{
    // set up a normal udp socket to revieve the response from the raw socket
    pair<int, struct sockaddr_in> connection = connect_to_port(addr, port);
    int udp_socket = connection.first;
    if (udp_socket < 0)
    {
        return -1;
    }
    struct sockaddr_in server_addr = connection.second;

    // extract the source address and port from the udp socket
    pair<string, int> source_ip_and_port = get_source_ip_and_port(udp_socket, server_addr);
    string source_ip = source_ip_and_port.first;
    int source_port = source_ip_and_port.second;

    // create a raw socket
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
    timeout.tv_sec = 1;
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
    iph->id = htonl(54321);
    iph->frag_off = 0x80; // set the evil bit
    iph->ttl = 255;
    iph->protocol = IPPROTO_UDP;
    iph->check = 0;
    iph->saddr = inet_addr(source_ip.c_str());
    iph->daddr = sin.sin_addr.s_addr;

    // Ip checksum
    iph->check = calculate_checksum((unsigned short *)datagram, iph->tot_len);

    // UDP header
    udph->source = htons(source_port);
    udph->dest = htons(port);
    udph->len = htons(sizeof(struct udphdr) + sizeof(secret_network_order));
    udph->check = 0;

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

    udph->check = calculate_checksum((unsigned short *)pseudogram, psize);
    free(pseudogram);

    int attempts = 0;
    int max_retries = 5;

    while (attempts < max_retries)
    {
        // send a message trough the raw socket with the evil bit set
        if (sendto(s, datagram, iph->tot_len, 0, (struct sockaddr *)&sin, sizeof(sin)) < 0)
        {
            perror("sendto failed");
        }
        else
        {
            char buffer[1024];
            memset(buffer, 0, sizeof(buffer));

            // Reviece the response on the udp socket
            if (recvfrom(udp_socket, buffer, sizeof(buffer), 0, NULL, NULL) < 0)
            {
                continue;
            }
            else
            {
                close(udp_socket);
                close(s);
                // extract the port from the response and return it
                // int secret_port = get_secret_port_from_buffer(buffer);
                string response(buffer);
                size_t pos = response.find_last_of(':');
                if (pos != string::npos)
                {
                    string number_str = response.substr(pos + 2, 5);
                    int extracted_port = stoi(number_str);
                    return extracted_port;
                }
            }
        }
        attempts++;
    }

    close(udp_socket);
    close(s);
    return -1;
}
