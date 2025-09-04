#include "include/loadConfig.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>

using namespace std;

int loadConfig(int &port,
               std::string &siteDir,
               std::string &Page404,
               bool &useDirListing,
               int &requestRateLimit,
               std::string &contactEmail,
               std::string &authCredentials,
               bool &toggleLogging,
               int &logMaxLines)
{
    std::ifstream envFile(".env");
    if (!envFile.is_open())
    {
        ofstream NewConfig(".env"); // no config

        NewConfig << "PORT=8080\n"
                       "SITE_DIR=public\n"
                       "404_PAGE=\n"
                       "DIR_LISTING=false\n"
                       "REQUEST_RATELIMIT=10\n"
                       "CONTACT_EMAIL=\n"
                       "AUTH_CREDENTIALS=\n"
                       "TOGGLE_LOGGING=false\n"
                       "LOG_MAX_LINES=5000\n"; // create default config

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
            if (value == "true")
            {
                useDirListing = true;
            }
            else
            {
                useDirListing = false;
            }
        }
        else if (key == "REQUEST_RATELIMIT") // requests/second rate limit per IP
        {
            int rl = std::atoi(value.c_str());
            if (rl >= 0) // 0 for none
                requestRateLimit = rl;
        }
        else if (key == "CONTACT_EMAIL") // contact email for error pages
        {
            contactEmail = value;
        }
        else if (key == "AUTH_CREDENTIALS") // HTTP Basic Auth credentials
        {
            authCredentials = value;
        }
        else if (key == "TOGGLE_LOGGING") // enable logging to .log file
        {
            for (auto &c : value)
                c = tolower(c);
            if (value == "true")
            {
                toggleLogging = true;
            }
            else
            {
                toggleLogging = false;
            }
        }
        else if (key == "LOG_MAX_LINES") // max lines in log file before rotation
        {
            int ll = std::atoi(value.c_str());
            if (ll >= 1) // at least 1 line
                logMaxLines = ll;
            else
            {
                logMaxLines = 0; // no limit
            }
        }
    }
    return 0;
}