#include "secret_port.h"

using namespace std;

// solve the secret port
pair<int, int> solve_secret_port(const string &addr, int port)
// Greetings from S.E.C.R.E.T (Secure Encryption Certification Relay with Enhanced Trust)! Here's how to access the secret port I'm safeguarding:
{
    // create a udp socket
    pair<int, struct sockaddr_in> connection = connect_to_port(addr, port);
    int sockfd = connection.first;
    struct sockaddr_in server_addr = connection.second;
    if (sockfd < 0)
    {
        return make_pair(-1, -1);
    }
    char buffer[1024];
    int attempts = 0;
    int max_retries = 5;
    uint8_t group_nr = 36;
    int group_secret = 0xfa899acb;
    while (attempts < max_retries)
    {
        // Send a message to the port
        //  1. Send me your group number as a single unsigned byte.
        if (sendto(sockfd, &group_nr, sizeof(group_nr), 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        {
            cerr << "Failed to send message to IP address." << endl;
            close(sockfd);
            return make_pair(-1, -1);
        }

        // Wait for a response.
        int bytes_received = recvfrom(sockfd, buffer, sizeof(buffer), 0, NULL, NULL);
        if (bytes_received >= 4)
        {
            // Extract the 4-byte challenge
            //  2. I'll reply with a 4-byte challenge (in network byte order) unique to your group.
            uint32_t challenge;
            memcpy(&challenge, buffer, sizeof(challenge));

            // Convert from network byte order to host byte order
            challenge = ntohl(challenge);

            //  3. Sign this challenge using the XOR operation with your group's secret (get that from your TA).
            uint32_t signature = challenge ^ group_secret;

            //  4. Reply with a 5-byte message: the first byte is your group number, followed by the 4-byte signed challenge (in network byte order).
            uint32_t network_order_four_byte = htonl(signature);
            uint8_t message[5];
            message[0] = group_nr;
            memcpy(&message[1], &network_order_four_byte, sizeof(network_order_four_byte));

            // Attempt to send the signed challenge and receive the response up to 5 times
            int inner_attempts = 0;
            while (inner_attempts < max_retries)
            {
                // Send the 5-byte message
                if (sendto(sockfd, message, sizeof(message), 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
                {
                    cerr << "Failed to send signed challenge to IP address." << endl;
                    close(sockfd);
                    return make_pair(-1, -1);
                }
                memset(buffer, 0, sizeof(buffer));

                //  5. If your signature is correct, I'll grant you access to the port. Good luck!
                if (recvfrom(sockfd, buffer, sizeof(buffer), 0, NULL, NULL) >= 0)
                {
                    close(sockfd);
                    // Extract the port number from the buffer and return it

                    string response(buffer);
                    size_t pos = response.find_last_of(':');
                    if (pos != string::npos)
                    {
                        string number_str = response.substr(pos + 2, 4);
                        int extracted_port = stoi(number_str);
                        return make_pair(extracted_port, signature);
                    }
                    // int secret_port = get_secret_port_from_buffer(buffer);
                    // return make_pair(secret_port, signature);
                }

                // try again if failed
                ++inner_attempts;
            }
            return make_pair(-1, -1);
        }

        // try again if failed
        ++attempts;
    }

    // All 5 attempts have failed, return false
    close(sockfd);

    return make_pair(-1, -1);
}