#include "include/perMinute404.h"
#include <vector>
#include <ctime>
#include <string>
#include <algorithm>
#include <cstdio>
using namespace std;

struct PerMinute404 // stores 404s per minute for an IP
{
    string ip;
    int count;
    time_t timestamp;
};

// cant believe they named it after the guy from despicable me
static vector<PerMinute404> notFoundPerMinute;

static void clearOldEntries()
{
    time_t now = time(nullptr);
    notFoundPerMinute.erase(
        std::remove_if(notFoundPerMinute.begin(), notFoundPerMinute.end(),
                       [now](const PerMinute404 &entry)
                       {
                           return (now - entry.timestamp) > 60;
                       }),
        notFoundPerMinute.end());
}

int get404PMcount(const string &ip)
{
    // clear old entries first
    clearOldEntries();

    int total = 0;
    for (const auto &entry : notFoundPerMinute)
    {
        if (entry.ip == ip)
        {
            total += entry.count;
        }
    }
    return total;
}

void add404PMentry(const string &ip)
{
    // clear old entries first
    time_t now = time(nullptr);
    clearOldEntries();


    // check if entry for this ip already exists
    for (auto &entry : notFoundPerMinute)
    {
        if (entry.ip == ip)
        {
            entry.count++;
            entry.timestamp = now; // update timestamp to now
            return;
        }
    }

    // add new entry
    notFoundPerMinute.push_back({ip, 1, now});
}