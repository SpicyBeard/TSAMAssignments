#include "oracle_port.h"

int solve_oracle_port(const string &addr, int port, int secret_secret_port, int dark_secret_port, int signature, string secret_phrase)
// todo
// Greetings! I am E.X.P.S.T.N, which stands for "Enhanced X-link Port Storage Transaction Node".
// What can I do for you?
// - If you provide me with a list of secret ports (comma-separated), I can guide you on the exact sequence of "knocks" to ensure you score full marks.
// How to use E.X.P.S.T.N?
// 1. Each "knock" must be paired with both a secret phrase and your unique S.E.C.R.E.T signature.
// 2. The correct format to send a knock: First, 4 bytes containing your S.E.C.R.E.T signature, followed by the secret phrase.
// Tip: To discover the secret ports and their associated phrases, start by solving challenges on the ports detected using your port scanner. Happy hunting!
{
    cout << "\nSolving Oracle port" << endl;
    // create the comma seperated string of ports
    string secret_ports = to_string(dark_secret_port) + "," + to_string(secret_secret_port);
    // set up the socket
    pair<int, struct sockaddr_in> connection = connect_to_port(addr, port);
    int sockfd = connection.first;
    struct sockaddr_in server_addr = connection.second;
    if (sockfd < 0)
    {
        return -1;
    }

    uint32_t message = htonl(signature);

    string knockSequence = send_and_receive(sockfd, secret_ports.c_str(), secret_ports.size(), server_addr, 5);
    close(sockfd);
    if (knockSequence == "")
    {
        return -1;
    }
    // create a vector of the ports recieved
    vector<int> secret_ports_vector;

    size_t start = 0;
    size_t end = knockSequence.find(',');

    while (end != string::npos)
    {
        secret_ports_vector.push_back(stoi(knockSequence.substr(start, end - start)));
        start = end + 1;
        end = knockSequence.find(',', start);
    }

    // set up the knock message
    secret_ports_vector.push_back(stoi(knockSequence.substr(start, end)));
    char knock_phrase[4 + secret_phrase.length()];
    memcpy(knock_phrase, &message, 4);
    memcpy(knock_phrase + 4, secret_phrase.c_str(), secret_phrase.length());

    string response;
    // go over all the ports and knock with the secret phrase
    for (int secret_port : secret_ports_vector)
    {
        pair<int, struct sockaddr_in> connection = connect_to_port(addr, secret_port);
        int secret_sockfd = connection.first;
        struct sockaddr_in server_addr = connection.second;
        if (secret_sockfd < 0)
        {
            return -1;
        }
        response = send_and_receive(secret_sockfd, knock_phrase, 4 + secret_phrase.length(), server_addr, 5);
        close(secret_sockfd);
    }
    cout << response << endl;
    return 1;
}
