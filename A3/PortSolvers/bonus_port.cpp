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
bool send_bonus_message(const string &addr, int port)
{
    cout << "Solving bonus port" << endl;
    int sockfd;
    struct sockaddr_in dest_addr;
    struct icmphdr icmp_hdr;
    const char *data = "$group_36$";
    // const char *data = "\"$group_36$\"";
    int data_len = strlen(data);
    char packet[sizeof(struct icmphdr) + data_len];

    // Create raw socket
    sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd < 0)
    {
        perror("socket");
        return false;
    }

    // Set destination address
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, addr.c_str(), &dest_addr.sin_addr) <= 0)
    {
        perror("inet_pton");
        close(sockfd);
        return false;
    }
    // Set a timeout for the socket
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
    {
        perror("Failed to set socket receive timeout");
        close(sockfd);
        return false;
    }

    // Prepare ICMP header
    memset(&icmp_hdr, 0, sizeof(icmp_hdr));
    icmp_hdr.type = ICMP_ECHO;
    icmp_hdr.code = 0;
    icmp_hdr.un.echo.id = getpid();
    icmp_hdr.un.echo.sequence = 1;

    // Copy ICMP header and data to packet
    memcpy(packet, &icmp_hdr, sizeof(icmp_hdr));
    memcpy(packet + sizeof(struct icmphdr), data, data_len);

    // Calculate checksum
    icmp_hdr.checksum = calculate_checksum((unsigned short *)packet, sizeof(packet));
    memcpy(packet, &icmp_hdr, sizeof(icmp_hdr)); // Update packet with checksum

    string response = send_and_receive(sockfd, packet, sizeof(packet), dest_addr, 5);
    cout << "ICMP echo sent" << endl;
    close(sockfd);
    return true;
}
