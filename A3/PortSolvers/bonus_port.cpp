#include "bonus_port.h"



struct psuedo_header
{
    u_int32_t source_address;
    u_int32_t dest_address;
    u_int8_t placeholder;
    u_int8_t protocol;
    u_int16_t udp_length;
};

// send the message to the bonus ip
void send_bonus_message(const string &addr, int port)
{
    int sockfd;
    struct sockaddr_in server_addr ;
    char datagram[4096], *data, *pseudogram;
    struct iphdr *iph = (struct iphdr *)datagram;
    struct icmphdr *icmph = (struct icmphdr *)(datagram + sizeof(struct iphdr));
    struct sockaddr_in sin;

    struct pseudo_header psh;

    // Create raw socket
    sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd < 0)
    {
        cerr << "Socket creation failed." << endl;
        return ;
    }

    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    sin.sin_addr.s_addr = inet_addr(addr.c_str());


    // Zero out the packet buffer
    memset(datagram, 0, 4096);

    // Fill in the ICMP Header
    icmph->type = ICMP_ECHO;
    icmph->code = 0;
    icmph->un.echo.id = htons(1234); // Identifier
    icmph->un.echo.sequence = htons(1); // Sequence number
    icmph->checksum = 0; // Set to 0 before calculating checksum
    icmph->checksum = calculate_checksum((unsigned short *)icmph, sizeof(struct icmphdr));

    // fill the data with our group number
    strcpy(data, "group36");


    // Fill in the IP Header
    iph->ihl = 5;
    iph->version = 4;
    iph->tos = 0;
    iph->tot_len = sizeof(struct iphdr) + sizeof(struct icmphdr) + strlen(data);
    iph->id = htonl(54321); // ID of this packet
    iph->frag_off = 0;
    iph->ttl = 255;
    iph->protocol = IPPROTO_ICMP;
    iph->check = 0; // Set to 0 before calculating checksum
    iph->saddr = inet_addr("0"); // Source IP address
    iph->daddr = server_addr.sin_addr.s_addr;

    // IP checksum
    iph->check = calculate_checksum((unsigned short *)datagram, iph->tot_len);


    // Send the packet
    if (sendto(sockfd, datagram, iph->tot_len, 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        cerr << "Failed to send packet." << endl;
        close(sockfd);
        return ;
    }

    cout << "Packet sent successfully." << endl;

    close(sockfd);
    return ;
}
