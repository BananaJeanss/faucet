#include "include/returnDirListing.h"
#include "include/return404.h"
#include <string>
#include <dirent.h>
#include <cstring>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>
#include <algorithm>

using namespace std;

static const char *pageHeader =
    "<!DOCTYPE html><html><head><meta charset=\"utf-8\"><title>Index of {Dir}</title>"
    "<style>body{font-family:sans-serif}table{border-collapse:collapse}td{padding:2px 8px;border-bottom:1px solid #ddd}</style>"
    "</head><body><h1>Index of {Dir}</h1><table>";

static const char *pageFooter = "</table></body></html>";

void returnDirListing(int client_fd,
                      const std::string &siteDir,
                      const std::string &relPath,
                      const std::string &Page404)
{
    std::string fullPath = siteDir.empty() ? relPath : (siteDir + "/" + relPath);

    DIR *dir = opendir(fullPath.c_str());
    if (!dir)
    {
        return404(client_fd, siteDir, Page404);
        return;
    }

    std::vector<std::string> entries;
    while (auto *ent = readdir(dir))
    {
        const char *name = ent->d_name;
        if (strcmp(name, ".") == 0)
            continue;
        entries.emplace_back(name);
    }
    closedir(dir);
    std::sort(entries.begin(), entries.end());

    std::string dirLabel = relPath.empty() ? "/" : relPath;
    std::string header = pageHeader;
    {
        const string needle = "{Dir}";
        size_t pos = 0;
        while ((pos = header.find(needle, pos)) != std::string::npos)
        {
            header.replace(pos, needle.length(), dirLabel);
            pos += dirLabel.length();
        }
    } // replace {Dir} with folder name

    std::string body = header;
    if (!relPath.empty())
    {
        body += "<tr><td><a href=\"../\">../</a></td></tr>";
    }
    for (auto &e : entries)
    {
        std::string display = e;
        body += "<tr><td><a href=\"";
        body += e;
        // append slash visually for directories (simple heuristic)
        body += "\">" + display + "</a></td></tr>";
    }
    body += pageFooter;

    char hdr[256];
    int hdrLen = snprintf(hdr, sizeof(hdr),
                          "HTTP/1.1 200 OK\r\n"
                          "Content-Length: %zu\r\n"
                          "Content-Type: text/html\r\n"
                          "Connection: close\r\n"
                          "\r\n",
                          body.size());
    if (hdrLen <= 0 || hdrLen >= (int)sizeof(hdr))
    {
        return404(client_fd, siteDir, Page404);
        return;
    }
    send(client_fd, hdr, hdrLen, 0);
    send(client_fd, body.data(), body.size(), 0);
    close(client_fd);
}