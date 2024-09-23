#ifndef CHECKSUM_PORT_H
#define CHECKSUM_PORT_H

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

// extract the secret phrase from the buffer
string get_secret_phrase(const char *buffer);
// solve the checksum port
string solve_checksum_port(const string &addr, int port, uint32_t secret);

#endif // CHECKSUM_PORT_H