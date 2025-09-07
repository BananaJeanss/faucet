#include "include/evaluateTrust.h"
#include "include/perMinute404.h"
#include <vector>
#include <ctime>
#include <algorithm>
#include <cstdio>

struct TrustScoreCacheEntry
{
    string ip;
    int trustScore;
    time_t timestamp;
    string userAgent; // if useragent differs, re-eval
};

static vector<TrustScoreCacheEntry> trustScoreCache;

struct requestPerMinute // storing this here for now cause nothing else outside would need to know requests per minute
{
    string ip;
    int count;
    time_t timestamp;
};

static vector<requestPerMinute> requestsPerMinute;

static int clearCache(int &trustScoreCacheDuration)
{
    time_t now = time(nullptr);
    try
    {
        trustScoreCache.erase(
            std::remove_if(trustScoreCache.begin(), trustScoreCache.end(),
                           [now, &trustScoreCacheDuration](const TrustScoreCacheEntry &entry)
                           {
                               return (now - entry.timestamp) > trustScoreCacheDuration;
                           }),
            trustScoreCache.end());
    }
    catch (...)
    {
        // if errors, log error, and if cache is reasonably big, clear it all
        // ngl idk if it even can error but try catch block ftw
        if (trustScoreCache.size() > 1000)
            trustScoreCache.clear();
        printf("Error in trust score cache cleanup\n");
        return 1; // error
    }
    return 0;
}

int evaluateTrust(const string &ip, const string &headers, int &trustScoreCacheDuration, bool &checkHoneypotPaths)
{
    // clear cache of old entries
    clearCache(trustScoreCacheDuration);

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

    // check cache first
    for (const auto &entry : trustScoreCache)
    {
        if (entry.ip == ip && entry.userAgent == userAgent)
        {
            // cached, but also check for 404s per minute cause if we cache on the first request, how tf we know if they did 404s after that
            int PM404Count = get404PMcount(ip);
            int cachedScore = entry.trustScore;
            if (PM404Count > 20) {
                cachedScore -= 40; // lots of 404s recently, lower trust
            }
            else if (PM404Count > 15) {
                cachedScore -= 25; // many 404s recently, lower trust
            } else if (PM404Count > 10) {
                cachedScore -= 15; // some 404s recently, lower trust a bit
            }
            else if (PM404Count > 5) {
                cachedScore -= 5; // few 404s recently, lower trust a bit
            }
            return cachedScore;
        }
    }

    // not in cache, evaluate trust score
    int score = 50; // Trust score, higher is more trusted. Start at a reasonable trust level

    // wont check for 404s here cause this is the first eval

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

    for (const auto &agent : suspiciousAgents)
    {
        if (userAgent.find(agent) != string::npos)
        {
            score -= 10; // suspicious user agent, lower trust
        }
    }

    for (const auto &agent : trustedAgents)
    {
        if (userAgent.find(agent) != string::npos)
        {
            score += 10; // trusted user agent, raise trust
        }
    }

    // check for sec-ch-ua-platform
    if (headers.find("sec-ch-ua-platform") != string::npos)
    {
        score += 5; // modern browser, raise trust a bit
    }
    else
    {
        score -= 5; // older browser or bot, lower trust a bit
    }

    // check for referer
    if (headers.find("Referer: ") != string::npos)
    {
        score += 5; // has referer, raise trust a bit, but dont lower if none
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

    if (rpm > 125)
    {
        score -= 20; // very high request rate, lower trust
    }
    else if (rpm > 75)
    {
        score -= 10; // high request rate, lower trust
    }
    else if (rpm > 35)
    {
        score -= 5; // moderate request rate, lower trust a bit
    }
    else if (rpm < 10)
    {
        score += 5; // low request rate, raise trust a bit
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

    // check if trying to access honeypot paths
    const vector<string> honeypotPaths = {
        "/admin",
        "/wp-login.php",
        "/xmlrpc.php",
        "/administrator",
        "/config.php",
        "/setup.php",
        "/install.php",
    };
    if (checkHoneypotPaths)
    {
        for (const auto &path : honeypotPaths)
        {
            if (headers.find("GET " + path) != string::npos || headers.find("POST " + path) != string::npos)
            {
                score -= 35; // accessing honeypot path, lower trust significantly
                break;
            }
        }
    }

    // clamp score to 0-100
    if (score < 0)
        score = 0;
    if (score > 100)
        score = 100;

    return score;
}