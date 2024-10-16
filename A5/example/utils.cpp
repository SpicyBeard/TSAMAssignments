#include "utils.h"

vector<string> checkMessageContentAndProcess(char *buffer)
{

    vector<string> tokens;
    if (buffer[0] == 0x01 && buffer[strlen(buffer) - 1] == 0x04)
    {
        string input(buffer);

        input.erase(0, 1); // Remove starting 0x01 marker

        input.erase(input.length() - 1); // Remove ending 0x04 marker

        // Trim any extra whitespaces, newline, etc.
        input.erase(0, input.find_first_not_of(" \n\r"));
        input.erase(input.find_last_not_of(" \n\r") + 1);
        string token;
        istringstream stream(input);

        while (getline(stream, token, ','))
        {
            tokens.push_back(token);
        }
        return tokens;
    }
    return tokens;
}

void logMessage(const std::string &msg)
{
    ofstream logfile;
    logfile.open(LOGFILE, ios::out | ios::app);
    if (!logfile.is_open())
    {
        cerr << "Failed to open log file" << endl;
        exit(1);
    }
    else
    {
        std::time_t now = std::time(0);
        char timeStr[100];
        strftime(timeStr, sizeof(timeStr), "%d-%m-%Y %H:%M:%S", localtime(&now));

        logfile << timeStr << ": " << msg << endl;
        cout << timeStr << ": " << msg << endl;
        logfile.close();
    }
}
