#include <string.h>
#include <iostream>
#include <fstream>
#include <ctime>
#include <vector>
#include <fstream>
#include <sstream>

using namespace std;
#define LOGFILE "server.log"

vector<string> checkMessageContentAndProcess(char *buffer);

void logMessage(const std::string &msg);
