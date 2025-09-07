#pragma once
#include <string>

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
               int &trustScoreCacheDuration,
               bool &checkHoneypotPaths,
               int &blockforDuration);