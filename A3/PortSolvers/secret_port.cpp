#include "secret_port.h"

using namespace std;

// solve the secret port
pair<int, int> solve_secret_port(const string &addr, int port)
// Greetings from S.E.C.R.E.T (Secure Encryption Certification Relay with Enhanced Trust)! Here's how to access the secret port I'm safeguarding:
{
    cout << "Solving secret port  " << endl;
    // create a udp socket
    pair<int, struct sockaddr_in> connection = connect_to_port(addr, port);
    int sockfd = connection.first;
    struct sockaddr_in server_addr = connection.second;
    if (sockfd < 0)
    {
        return make_pair(-1, -1);
    }

    uint8_t group_nr = 36;
    int group_secret = 0xfa899acb;
    string challenge = send_and_receive(sockfd, &group_nr, sizeof(group_nr), server_addr, 5);

    // Extract the 4-byte challenge
    //  2. I'll reply with a 4-byte challenge (in network byte order) unique to your group.
    // change challenge into uint32_t
    uint32_t challenge_int = 0;
    memcpy(&challenge_int, challenge.c_str(), sizeof(challenge_int));

    // Convert from network byte order to host byte order
    challenge_int = ntohl(challenge_int);

    //  3. Sign this challenge using the XOR operation with your group's secret (get that from your TA).
    uint32_t signature = challenge_int ^ group_secret;

    //  4. Reply with a 5-byte message: the first byte is your group number, followed by the 4-byte signed challenge (in network byte order).
    uint32_t network_order_four_byte = htonl(signature);
    uint8_t message[5];
    message[0] = 36;
    memcpy(&message[1], &network_order_four_byte, sizeof(network_order_four_byte));

    // int to string message

    // Send the 5-byte message
    string response = send_and_receive(sockfd, message, sizeof(message), server_addr, 5);
    if (response != "")
    {
        cout << response << "\n"
             << endl;
        size_t pos = response.find_last_of(':');
        if (pos != string::npos)
        {
            string number_str = response.substr(pos + 2, 4);
            int extracted_port = stoi(number_str);
            close(sockfd);
            return make_pair(extracted_port, signature);
        }
    }

    // All 5 attempts have failed, return false
    close(sockfd);

    return make_pair(-1, -1);
}