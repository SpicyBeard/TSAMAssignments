#ifndef ORACLE_PORT_H
#define ORACLE_PORT_H

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

bool solve_expstn_port(const string &addr, int port, int secret_secret_port, int dark_secret_port, int signature, string secret_phrase);

#endif // ORACLE_PORT_H