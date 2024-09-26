#ifndef BONUS_PORT_H
#define BONUS_PORT_H

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
#include <netinet/ip_icmp.h>
#include "../common.h"

using namespace std;

// send the message to the bonus ip
bool send_bonus_message(const string &addr, int port);

#endif // BONUS_PORT_H