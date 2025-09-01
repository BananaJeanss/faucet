#include "include/loadConfig.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>

using namespace std;

int loadConfig(int &port, std::string &siteDir, std::string &Page404, bool &useDirListing, int &requestRateLimit)
{
    std::ifstream envFile(".env");
    if (!envFile.is_open())
    {
        ofstream NewConfig(".env"); // no config

        NewConfig << "PORT=8080\nSITE_DIR=public\n404_PAGE=\nDIR_LISTING=false\nREQUEST_RATELIMIT=10";

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
        else if (key == "404_PAGE") // load custom 404 page
        {
            Page404 = value;
        }
        else if (key == "DIR_LISTING") // enable directory listing
        {
            // lowercase key
            for (auto &c : value)
                c = tolower(c);
            if (value == "true") {
                useDirListing = true;
            } else {
                useDirListing = false;
            }
        }
        else if (key == "REQUEST_RATELIMIT") // requests/second rate limit per IP
        {
            int rl = std::atoi(value.c_str());
            if (rl >= 0) // 0 for none
                requestRateLimit = rl;
        }
    }
    return 0;
}