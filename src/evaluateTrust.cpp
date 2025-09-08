#include "include/evaluateTrust.h"
#include "include/perMinute404.h"
#include <vector>
#include <ctime>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cctype>

struct requestPerMinute // storing this here for now cause nothing else outside would need to know requests per minute
{
    string ip;
    int count;
    time_t timestamp;
};

static vector<requestPerMinute> requestsPerMinute;

const vector<string> defaultHoneypotPaths = {
    "/admin",
    "/wp-login.php",
    "/xmlrpc.php",
    "/administrator",
    "/config.php",
    "/setup.php",
    "/install.php",
};

vector<string> honeypotPaths = defaultHoneypotPaths;

struct lowestScorePerMinute
{ // keeps track of the lowest score per IP per minute
    string ip;
    int score;
    time_t timestamp;
};

static vector<lowestScorePerMinute> lowestScores;

struct honeypotsPer3Minutes
{
    string ip;
    int count;
    time_t timestamp;
};

static vector<honeypotsPer3Minutes> honeypots;

static void clearLowestScores() // clears entries older than 1 minute
{
    time_t now = time(nullptr);
    lowestScores.erase(
        std::remove_if(lowestScores.begin(), lowestScores.end(),
                       [now](const lowestScorePerMinute &entry)
                       {
                           return (now - entry.timestamp) > 60;
                       }),
        lowestScores.end());
}

static int checkLowestScore(const string &ip)
{
    auto it = std::find_if(lowestScores.begin(), lowestScores.end(),
                           [&ip](const lowestScorePerMinute &entry)
                           {
                               return entry.ip == ip;
                           });
    if (it != lowestScores.end())
    {
        return it->score;
    }
    return -1; // not found
}

static void clearHoneypotCounts() // clears entries older than 3 minutes
{
    time_t now = time(nullptr);
    honeypots.erase(
        std::remove_if(honeypots.begin(), honeypots.end(),
                       [now](const honeypotsPer3Minutes &entry)
                       {
                           return (now - entry.timestamp) > 180;
                       }),
        honeypots.end());
}

static int getHoneypotPMcount(const string &ip)
{
    clearHoneypotCounts();
    int count = 0;
    for (const auto &entry : honeypots)
    {
        if (entry.ip == ip)
        {
            count += entry.count;
        }
    }
    return count;
}

static void addHoneypotHit(const string &ip)
{
    time_t now = time(nullptr);
    auto it = std::find_if(honeypots.begin(), honeypots.end(),
                           [&ip](const honeypotsPer3Minutes &entry)
                           {
                               return entry.ip == ip;
                           });
    if (it != honeypots.end())
    {
        it->count++;
        it->timestamp = now; // update timestamp to extend the window
    }
    else
    {
        honeypots.push_back({ip, 1, now});
    }
}

void initializeHoneypotPaths()
{
    // if honeypotPaths.txt exists in same dir as exe, load paths from there per line
    FILE *file = fopen("honeypotPaths.txt", "r");
    if (file)
    {
        // clear honeypotPaths cause the defaults are already in there
        honeypotPaths.clear();

        // read lines
        char line[4096];
        while (fgets(line, sizeof(line), file))
        {
            // Remove trailing newline and carriage return and surrounding spaces/tabs
            size_t len = strlen(line);
            while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r' || line[len - 1] == ' ' || line[len - 1] == '\t'))
            {
                line[len - 1] = '\0';
                --len;
            }
            size_t start = 0;
            while (line[start] == ' ' || line[start] == '\t')
                ++start;
            if (len > start)
            {
                std::string cleaned = std::string(line + start, len - start);
                if (!cleaned.empty() && cleaned[0] != '/')
                {
                    // ensure paths start with '/'
                    cleaned = "/" + cleaned;
                }
                honeypotPaths.push_back(cleaned);
            }
        }
        fclose(file);
        printf("Loaded %zu honeypot paths from honeypotPaths.txt\n", honeypotPaths.size());
    }
    else
    {
        printf("honeypotPaths.txt not found, using default honeypot paths\n");
    }
}

int evaluateTrust(const string &ip, const string &headers, bool &checkHoneypotPaths)
{
    // store request in requestsPerMinute
    time_t now = time(nullptr);
    requestsPerMinute.push_back({ip, 1, now});

    // remove old entries from requestsPerMinute
    requestsPerMinute.erase(
        std::remove_if(requestsPerMinute.begin(), requestsPerMinute.end(),
                       [now](const requestPerMinute &entry)
                       {
                           return (now - entry.timestamp) > 60;
                       }),
        requestsPerMinute.end());

    // extract user agent from headers
    string userAgent;
    size_t userAgentStart = headers.find("User-Agent: ");
    if (userAgentStart != string::npos)
    {
        size_t userAgentEnd = headers.find("\r\n", userAgentStart);
        userAgent = headers.substr(userAgentStart + 12, userAgentEnd - userAgentStart - 12);
    }

    int score = 30; // Trust score, higher is more trusted. Start at a reasonable trust level

    // user agent stuff
    if (userAgent.empty())
    {
        score -= 20; // no user agent, lower trust
    }

    // suspicious user agents
    const vector<string> suspiciousAgents = {
        "curl", "wget", "python-requests", "libwww-perl", "java", "php", "ruby", "scrapy", "httpclient", "go-http-client"};

    // trusted(?) user agents, can be faked but whatever
    const vector<string> trustedAgents = {
        "Mozilla", "Chrome", "Safari", "Edge", "Firefox", "Opera", "AppleWebKit", "Gecko"};

    bool agentFoundAlready = false; // prevents double counting
    bool isUnknownAgent = true;
    for (const auto &agent : suspiciousAgents)
    {
        if (agentFoundAlready)
            break;
        if (userAgent.find(agent) != string::npos)
        {
            score -= 10; // suspicious user agent, lower trust
            isUnknownAgent = false;
            agentFoundAlready = true;
        }
    }

    for (const auto &agent : trustedAgents)
    {
        if (agentFoundAlready)
            break;
        if (userAgent.find(agent) != string::npos)
        {
            score += 10; // trusted user agent, raise trust
            isUnknownAgent = false;
            agentFoundAlready = true;
        }
    }

    if (isUnknownAgent)
    {
        score -= 10; // unknown user agent
    }

    const vector<string> legitPlatforms = {
        "Windows", "Linux", "macOS", "Android", "iOS", "Chrome OS", "Chromium OS"};

    // check for sec-ch-ua-platform
    if (headers.find("sec-ch-ua-platform") != string::npos)
    {
        // check if actually legit
        bool foundLegit = false;
        for (const auto &plat : legitPlatforms)
        {
            if (headers.find(plat) != string::npos)
            {
                foundLegit = true;
                break;
            }
        }
        if (foundLegit)
        {
            score += 5; // has legit platform, raise trust
        }
        else if (headers.find("Unknown") != string::npos)
        {
            // unknown platform, at least included so dont lower/raise
        }
        else
        {
            score -= 5; // older browser or bot, lower trust a bit
        }
    }

    // check for referer
    if (headers.find("Referer: ") != string::npos)
    {
        score += 5; // has referer, raise trust a bit, but dont lower if none
    }

    // accept-encoding lalala
    if (headers.find("Accept-Encoding: ") != string::npos)
    {
        // make sure it has e.g. gzip or deflate
        if (headers.find("gzip") != string::npos || headers.find("deflate") != string::npos || headers.find("br") != string::npos || headers.find("zstd") != string::npos)
        {
            score += 5; // has, raise trust a bit
        }
    }
    else
    {
        score -= 5; // no accept-encoding, lower trust a bit
    }

    // check for requests per minute from this IP
    int rpm = 0;
    for (const auto &entry : requestsPerMinute)
    {
        if (entry.ip == ip)
        {
            rpm += entry.count;
        }
    }

    // requests per minute checks
    if (rpm > 60)
    {
        score -= 20; // very high request rate, lower trust
    }
    else if (rpm > 45)
    {
        score -= 10; // high request rate, lower trust
    }
    else if (rpm > 25)
    {
        score -= 5; // moderate request rate, lower trust a bit
    }

    // ip range checks (basic)
    if (ip.find("192.168.") == 0 || ip.find("10.") == 0 || ip.find("172.") == 0)
    {
        score += 15; // private IP range, increase trust
    }
    else if (ip.find("127.") == 0 || ip == "::1")
    {
        score += 35; // localhost, very high trust
    }

    // Extract request line to get exact path (first line up to CRLF)
    string requestLine;
    {
        size_t lineEnd = headers.find("\r\n");
        if (lineEnd != string::npos)
            requestLine = headers.substr(0, lineEnd);
        else
            requestLine = headers; // fallback if malformed
    }
    string method, reqPath;
    {
        // simple split by spaces
        size_t firstSpace = requestLine.find(' ');
        if (firstSpace != string::npos)
        {
            method = requestLine.substr(0, firstSpace);
            size_t secondSpace = requestLine.find(' ', firstSpace + 1);
            if (secondSpace != string::npos)
            {
                reqPath = requestLine.substr(firstSpace + 1, secondSpace - firstSpace - 1);
            }
        }
    }

    // normalize reqPath: remove trailing slashes except root
    if (reqPath.size() > 1)
    {
        while (!reqPath.empty() && reqPath.size() > 1 && reqPath.back() == '/')
            reqPath.pop_back();
    }

    if (checkHoneypotPaths && !reqPath.empty())
    {
        for (const auto &path : honeypotPaths)
        {
            // also normalize the honeypot path similarly (assume stored normalized)
            string hp = path;
            if (hp.size() > 1)
            {
                while (hp.size() > 1 && hp.back() == '/')
                    hp.pop_back();
            }
            if (reqPath == hp)
            {
                score -= 35; // accessing honeypot path, lower trust significantly
                addHoneypotHit(ip);
                break;
            }
        }
    }

    // honeypots per 3 minutes check
    if (checkHoneypotPaths)
    {
        int hpCount = getHoneypotPMcount(ip);
        if (hpCount >= 7)
        {
            score -= 65; // very high honeypot access rate, lower trust heavily (maybe block outright instead but idk)
        }
        else if (hpCount >= 6)
        {
            score -= 40; // high honeypot access rate, lower trust
        }
        else if (hpCount >= 4)
        {
            score -= 25; // moderate honeypot access rate, lower trust
        }
        else if (hpCount >= 2)
        {
            score -= 10; // low honeypot access rate, could just be a mistake, lower trust a bit
        }
    }

    // check 404s per minute from this IP
    int f404 = get404PMcount(ip);
    if (f404 > 30)
    {
        score -= 35; // very high 404 rate, lower trust significantly
    }
    else if (f404 > 20)
    {
        score -= 20; // high 404 rate, lower trust
    }
    else if (f404 > 10)
    {
        score -= 10; // moderate 404 rate, lower trust a bit
    }

    // clamp score to 0-100
    if (score < 0)
        score = 0;
    if (score > 100)
        score = 100;

    // check if score is lower than previous lowest in last minute
    clearLowestScores();
    int prevLowest = checkLowestScore(ip);
    int finalScore = score;
    if (prevLowest == -1)
    {
        // first entry for this IP in current window
        lowestScores.push_back({ip, score, now});
    }
    else if (score < prevLowest)
    {
        // new lower score replaces stored
        for (auto &entry : lowestScores)
        {
            if (entry.ip == ip)
            {
                entry.score = score;
                entry.timestamp = now;
                break;
            }
        }
    }
    else
    {
        // use previous lowest score
        finalScore = prevLowest;
    }

    if (finalScore != score)
    {
        printf("Evaluated trust score for %s: %d (using previous lowest, raw: %d)\n", ip.c_str(), finalScore, score);
    }
    else
    {
        printf("Evaluated trust score for %s: %d\n", ip.c_str(), finalScore);
    }

    return finalScore;
}