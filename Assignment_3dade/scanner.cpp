#include <iostream>
#include <cstring>  // for memset()
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>  // for close()


int main (char ipAddress, int port1, int por2 )

unique_ptr
int udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
if (udp_socket < 0) {
    std::cerr << "Failed to create socket" << std::endl;
    return 1;
}


