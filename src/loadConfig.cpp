#include "include/loadConfig.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>

using namespace std;

int loadConfig(int &port, std::string &siteDir)
{
    std::ifstream envFile(".env");
    if (!envFile.is_open())
    {
        ofstream NewConfig(".env"); // no config

        NewConfig << "PORT=8080\n"; // basic for now

        NewConfig.close();
        return 2;
    }

    std::string line;
    while (std::getline(envFile, line))
    {
        if (line.empty() || line[0] == '#')
            continue; // skip comments/blank lines

        size_t eqPos = line.find('=');
        if (eqPos == std::string::npos)
            continue; // skip malformed line

        std::string key = line.substr(0, eqPos);
        std::string value = line.substr(eqPos + 1);

        if (key == "PORT") // load port, command arg overrides
        {
            int p = std::atoi(value.c_str());
            if (p >= 1 && p <= 65535)
                port = p;
        }
        else if (key == "SITE_DIR") // load site directory
        {
            siteDir = value;
        }
    }
    return 0;
}