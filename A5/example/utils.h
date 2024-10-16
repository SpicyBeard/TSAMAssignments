#include <string.h>
#include <iostream>
#include <fstream>
#include <ctime>

using namespace std;
#define LOGFILE "server.log"

string checkMessageContent(char *buffer, int clientSocket);

void logMessage(const std::string &msg);