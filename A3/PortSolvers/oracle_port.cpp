#include "oracle_port.h"

bool solve_expstn_port(const string &addr, int port, int secret_secret_port, int dark_secret_port, int signature, string secret_phrase)
// todo
// Greetings! I am E.X.P.S.T.N, which stands for "Enhanced X-link Port Storage Transaction Node".
// What can I do for you?
// - If you provide me with a list of secret ports (comma-separated), I can guide you on the exact sequence of "knocks" to ensure you score full marks.
// How to use E.X.P.S.T.N?
// 1. Each "knock" must be paired with both a secret phrase and your unique S.E.C.R.E.T signature.
// 2. The correct format to send a knock: First, 4 bytes containing your S.E.C.R.E.T signature, followed by the secret phrase.
// Tip: To discover the secret ports and their associated phrases, start by solving challenges on the ports detected using your port scanner. Happy hunting!
{
    // create the comma seperated string of ports
    string secret_ports = to_string(dark_secret_port) + "," + to_string(secret_secret_port);
    // set up the socket
    pair<int, struct sockaddr_in> connection = connect_to_port(addr, port);
    int sockfd = connection.first;
    struct sockaddr_in server_addr = connection.second;
    if (sockfd < 0)
    {
        return "";
    }

    char buffer[1024];

    int attempts = 0;
    int max_retries = 5;
    uint32_t message = htonl(signature);

    while (attempts < max_retries)
    {
        // send a message to the port containing the signature from the secret port
        if (sendto(sockfd, secret_ports.c_str(), secret_ports.size(), 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        {
            cerr << "Failed to send message to IP address first." << endl;
            close(sockfd);
            return "";
        }

        memset(buffer, 0, sizeof(buffer));
        // Wait for a response. if there is a response, the port is open
        if (recvfrom(sockfd, buffer, sizeof(buffer), 0, NULL, NULL) < 0)
        {

            close(sockfd);
            return "";
        }
        else
        {
            // if there was a reponse, break from this loop and continue
            close(sockfd);
            break;
        }
        attempts++;
    }

    // create a vector of the ports recieved
    vector<int> secret_ports_vector;
    string buffer_str(buffer);
    size_t start = 0;
    size_t end = buffer_str.find(',');

    while (end != string::npos)
    {
        secret_ports_vector.push_back(stoi(buffer_str.substr(start, end - start)));
        start = end + 1;
        end = buffer_str.find(',', start);
    }

    // NOTE: the knocking message might not be right.
    // set up the knock message
    secret_ports_vector.push_back(stoi(buffer_str.substr(start, end)));
    unsigned char* knocked_phrase =  new unsigned char[4 + secret_phrase.length()];
    memcpy(knocked_phrase, &message, 4);
    memcpy(knocked_phrase + 4, secret_phrase.c_str(), secret_phrase.length());

    // go over all the ports and knock with the secret phrase
    for (int secret_port : secret_ports_vector)
    {
        //cout << "sending knock : " << knocked_phrase << " to port " << secret_port <<  endl;
        pair<int, struct sockaddr_in> connection = connect_to_port(addr, secret_port);
        int secret_sockfd = connection.first;
        struct sockaddr_in server_addr = connection.second;
        if (secret_sockfd < 0)
        {
            return "";
        }
        attempts = 0;
        while (attempts < max_retries)
        {
            // if (sendto(secret_sockfd, knock_phrase.data(), knock_phrase.length(), 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
            if (sendto(secret_sockfd, knocked_phrase, 4 + secret_phrase.length(), 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
            {
                cerr << "Failed to send message to IP address first." << endl;
            }

            memset(buffer, 0, sizeof(buffer));
            if (recvfrom(secret_sockfd, buffer, sizeof(buffer), 0, NULL, NULL) > 0)
            {
                cout << buffer << endl;
                close(secret_sockfd);
                break;
            }
            attempts++;
        }
    }
    if (recvfrom(sockfd, buffer, sizeof(buffer), 0, NULL, NULL) > 0)
    {
        cout << buffer << endl;
        cout << "done" << endl;
        close(sockfd);
    }
    else{
        cout << "failed" << endl;
        close(sockfd);

    }
    // All 5 attempts have failed, return false
    return "";
}
