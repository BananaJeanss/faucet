#pragma once
#include <string>

void returnDirListing(int client_fd,
                      const std::string &siteDir,
                      const std::string &relPath,
                      const std::string &Page404,
                      const std::string &contactEmail,
                      const std::string &ip);