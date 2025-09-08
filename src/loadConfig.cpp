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
               int &logMaxLines,
               bool &trustXRealIp,
               bool &evaluateTrustScore,
               int &trustScoreThreshold,
               bool &checkHoneypotPaths,
               int &blockforDuration)
{
    std::ifstream envFile(".env");
    if (!envFile.is_open())
    {
        ofstream NewConfig(".env"); // no config

        NewConfig << "PORT=8080\n" // create default config
                     "SITE_DIR=public\n"
                     "404_PAGE=\n"
                     "DIR_LISTING=false\n"
                     "REQUEST_RATELIMIT=10\n"
                     "CONTACT_EMAIL=\n"
                     "AUTH_CREDENTIALS=\n"
                     "TOGGLE_LOGGING=false\n"
                     "LOG_MAX_LINES=5000\n"
                     "TRUST_XREALIP=false\n"
                     "EVALUATE_TRUSTSCORE=false\n"
                     "TRUSTSCORE_THRESHOLD=10\n"
                     "CHECK_HONEYPOT_PATHS=false\n"
                     "BLOCKFOR_DURATION=600\n";

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
            if (value == "example") // example value, warn but continue
            {
                std::cerr << "Warning: SITE_DIR is set to 'example'. Please change this to your site's directory name." << std::endl;
            }
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
        else if (key == "TRUST_XREALIP") // trust X-Real-IP header from reverse proxy
        {
            for (auto &c : value)
                c = tolower(c);
            if (value == "true")
            {
                trustXRealIp = true;
            }
            else
            {
                trustXRealIp = false;
            }
        }
        // trust score stuff
        else if (key == "EVALUATE_TRUSTSCORE") // enable trust score evaluation
        {
            for (auto &c : value)
                c = tolower(c);
            if (value == "true")
            {
                evaluateTrustScore = true;
            }
            else
            {
                evaluateTrustScore = false;
            }
        }
        else if (key == "TRUSTSCORE_THRESHOLD") // trust score threshold for blocking
        {
            int tst = std::atoi(value.c_str());
            if (tst >= 0 && tst <= 100) // valid range
                trustScoreThreshold = tst;
            else trustScoreThreshold = 90; // default 90
        }
        else if (key == "CHECK_HONEYPOT_PATHS") // check for honeypot paths such as /admin, /wp-login.php, /xmlrpc.php, etc.
        {
            for (auto &c : value)
                c = tolower(c);
            if (value == "true")
            {
                checkHoneypotPaths = true;
            }
            else
            {
                checkHoneypotPaths = false;
            }
        }
        else if (key == "BLOCKFOR_DURATION") // duration in seconds to block an IP for if it goes below the trust score threshold
        {
            int bfd = std::atoi(value.c_str());
            if (bfd >= 0) // 0 for no blocking
                blockforDuration = bfd;
        }
    }
    return 0;
}