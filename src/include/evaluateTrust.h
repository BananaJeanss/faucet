#pragma once
#include <string>
using namespace std;

// evaluates trust, returns score in int, higher is better
int evaluateTrust(const string &ip, 
    const string &headers, 
    int &trustScoreCacheDuration, 
    bool &checkHoneypotPaths);