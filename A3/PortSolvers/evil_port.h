#ifndef EVIL_PORT_H
#define EVIL_PORT_H

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
#include "../common.h"

using namespace std;

// solve the evil port
int solve_evil_port(const string &addr, int port, int secret);
// get the source ip and port for the raw socket
pair<string, int> get_source_ip_and_port(int sockfd, struct sockaddr_in server_addr);

#endif // EVIL_PORT_H