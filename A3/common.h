#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <vector>
#include <map>
#include <cstring>
#include <utility>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netdb.h>

using namespace std;

// Connect to a port on a given IP address and return the socket file descriptor and server address
pair<int, struct sockaddr_in> connect_to_port(const string &addr, int port);

// Compute the checksum for a given buffer
unsigned short calculate_checksum(unsigned short *ptr, int nbytes);

// get source ip and port from a socket
pair<string, int> get_source_ip_and_port(int sockfd, struct sockaddr_in server_addr);

// send a message to a socket and receive a response
string send_and_receive(int sockfd, const void *message, size_t message_len, struct sockaddr_in &server_addr, int max_retries);

// pseudo heder struct
struct pseudo_header
{
    u_int32_t source_address;
    u_int32_t dest_address;
    u_int8_t placeholder;
    u_int8_t protocol;
    u_int16_t udp_length;
};

#endif // COMMON_H
