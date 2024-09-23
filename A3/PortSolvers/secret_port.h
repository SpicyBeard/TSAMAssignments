#ifndef SECRET_PORT_H
#define SECRET_PORT_H

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

// solve the secret port
std::pair<int, int> solve_secret_port(const std::string &ip_addr, int secret_port);

#endif // SECRET_PORT_H