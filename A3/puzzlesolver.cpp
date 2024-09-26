#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <utility>
#include <cstdlib>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "common.h"
#include "PortSolvers/secret_port.h"
#include "PortSolvers/checksum_port.h"
#include "PortSolvers/evil_port.h"
#include "PortSolvers/oracle_port.h"
#include "PortSolvers/bonus_port.h"

using namespace std;

// Check if a port is open and return the message received
string check_port(const string &addr, int port)
{
    pair<int, struct sockaddr_in> connection = connect_to_port(addr, port);
    int sockfd = connection.first;
    struct sockaddr_in server_addr = connection.second;
    if (sockfd < 0)
    {
        return "";
    }

    string message = send_and_receive(sockfd, "hello?", 7, server_addr, 5);
    if (message == "")
    {
        return "";
    }
    else
    {
        return message;
    }
}

// Sort the port based on the message received from them
string sort_port(string portmsg)
{
    if (portmsg.find("Enhanced X-link Port Storage Transaction Node") != string::npos)
    {
        return "expstn";
    }
    else if (portmsg.find("Secure Encryption Certification Relay with Enhanced Trust") != string::npos)
    {
        return "secret";
    }
    else if (portmsg.find("The dark side") != string::npos)
    {
        return "dark";
    }
    else
    {
        return "checksum";
    }
}

// Check if a port is open and sort it based on the message received
pair<string, int> check_and_sort_port(const string &ip_addr, int port)
{
    string port_msg = check_port(ip_addr, port);
    string port_type = sort_port(port_msg);
    return make_pair(port_type, port);
}

int main(int argc, char *argv[])
{
    if (argc != 6)
    {
        printf("Usage: puzzlesolver <IP address> <port1> <port2> <port3> <port4>");
        exit(0);
    }

    string ip_addr = argv[1];
    vector<int> ports = {atoi(argv[2]), atoi(argv[3]), atoi(argv[4]), atoi(argv[5])};

    // handle port sorting
    map<string, int> port_map;
    bool all_ports_found = false;

    while (!all_ports_found)
    {
        port_map.clear();
        for (int port : ports)
        {
            auto result = check_and_sort_port(ip_addr, port);
            port_map[result.first] = result.second;
        }

        all_ports_found = (port_map["secret"] != 0 && port_map["dark"] != 0 && port_map["checksum"] != 0 && port_map["expstn"] != 0);
    }

    int secret_port = port_map["secret"];
    int dark_port = port_map["dark"];
    int checksum_port = port_map["checksum"];
    int expstn_port = port_map["expstn"];

    int secret_secret_port, signature;
    while (true)
    {

        auto secret_response = solve_secret_port(ip_addr, secret_port);
        secret_secret_port = secret_response.first;
        signature = secret_response.second;
        if (secret_response.first != -1 && secret_response.second != -1)
        {
            break;
        }
    }
    // and solve in the correct order
    string secret_phrase;
    int dark_secret_port = -1;
    int solve_expstn = -1;
    bool send_bonus = false;
    while (secret_phrase == "")
    {
        secret_phrase = solve_checksum_port(ip_addr, checksum_port, signature);
    }
    while (dark_secret_port < 0)
    {
        dark_secret_port = solve_evil_port(ip_addr, dark_port, signature);
    }
    while (solve_expstn < 0)
    {
        solve_expstn = solve_expstn_port(ip_addr, expstn_port, secret_secret_port, dark_secret_port, signature, secret_phrase);
    }
    while (!send_bonus)
    {

        send_bonus = send_bonus_message(ip_addr, 4094);
    }
    cout << "All ports solved! \nThanks for the puzzles :)" << endl;
    return 0;
}
