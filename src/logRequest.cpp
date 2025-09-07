#include "include/logRequest.h"
#include "include/evaluateTrust.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cstdio>
#include <cstdlib>

using namespace std;

void logRequest(const string &consoleOutput,
                bool toggleLogging,
                int logMaxLines)
{
    // log to console
    cout << consoleOutput << endl;

    if (!toggleLogging)
        return;

    // log to file
    ofstream logFile("server.log", ios::app);
    if (!logFile.is_open())
        return;

    logFile << consoleOutput << endl;
    logFile.close();

    // check if we need to rotate
    ifstream inFile("server.log");
    if (!inFile.is_open())
        return;

    int lineCount = 0;
    string line;
    while (getline(inFile, line))
    {
        lineCount++;
    }
    inFile.close();

    if (logMaxLines == 0)
        return; // do nothing, 0 means no limit
    if (lineCount > logMaxLines)
    {
        // rotate to backup, overwrite existing backup
        int renameresult = rename("server.log", "server.log.bak");
        if (renameresult != 0)
        {
            perror("rename");
        }
        ofstream newLogFile("server.log");
        newLogFile.close();
    }
}