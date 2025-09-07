#include "include/returnDirListing.h"
#include "include/return404.h"
#include <string>
#include <dirent.h>
#include <cstring>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>
#include <algorithm>
#include <sys/stat.h>
#include <sys/types.h>
#include <ctime>

using namespace std;

static const char *pageHeader =
    "<!DOCTYPE html><html><head><meta charset=\"utf-8\"><title>Index of {Dir}</title>"
    "<style>body{font-family:sans-serif}table{border-collapse:collapse}td{padding:2px 8px;border-bottom:1px solid #ddd}</style>"
    "</head><body><h1>Index of {Dir}</h1><table>";

static const char *pageFooter = "</table></body></html>";

void returnDirListing(int client_fd,
                      const std::string &siteDir,
                      const std::string &relPath,
                      const std::string &Page404,
                      const std::string &contactEmail,
                      const std::string &ip)
{
    std::string fullPath = siteDir.empty() ? relPath : (siteDir + "/" + relPath);

    DIR *dir = opendir(fullPath.c_str());
    if (!dir)
    {
        return404(client_fd, siteDir, Page404, contactEmail, ip);
        return;
    }

    std::vector<std::string> entries;
    while (auto *ent = readdir(dir))
    {
        const char *name = ent->d_name;
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) // skip . and .., add manually
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
    body += "<hr>"; // hr for style points cause its cool
    body += "<tr><th>Name</th><th>Size</th><th>Date Modified</th></tr>";
    if (!relPath.empty())
    {
        body += "<tr><td><a href=\"/\">/</a></td></tr>"; // root
        body += "<tr><td><a href=\"../\">../</a></td></tr>"; // parent dir
    }

    for (auto &e : entries)
    {
        // check if dir, if file, add size data later
        bool isDir = false;
        {
            struct stat st;
            std::string p = fullPath + "/" + e;
            if (stat(p.c_str(), &st) == 0 && S_ISDIR(st.st_mode))
                isDir = true;
        }

        std::string display = e;
        if (isDir && display.back() != '/')
            display += '/';

        body += "<tr><td><a href=\"";
        body += e;
        if (isDir)
            body += "/"; // ensure link ends with slash for directories
        body += "\">" + display + "</a></td>";

        if (!isDir)
        {
            // Get file size
            struct stat st;
            if (stat((fullPath + "/" + e).c_str(), &st) == 0)
            {
                int bytes = st.st_size;
                // convert to human readable
                const char *units[] = {"B", "KB", "MB", "GB"};
                int unitIdx = 0;
                double fsize = bytes;
                while (fsize >= 1024.0 && unitIdx < 3)
                {
                    fsize /= 1024.0;
                    unitIdx++;
                }
                // round to 1 decimal place at most
                char sizeStr[32];
                if (unitIdx == 0)
                    snprintf(sizeStr, sizeof(sizeStr), "%d %s", bytes, units[unitIdx]);
                else
                    snprintf(sizeStr, sizeof(sizeStr), "%.1f %s", fsize, units[unitIdx]);
                body += "<td>" + std::string(sizeStr) + "</td>";

                // get date modified if available
                char timebuf[64];
                struct tm *tm_info = localtime(&st.st_mtime);
                strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M", tm_info);
                body += "<td>" + std::string(timebuf) + "</td>";
            }
        }

        // close
        body += "</tr>";
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
        return404(client_fd, siteDir, Page404, contactEmail, ip);
        return;
    }
    send(client_fd, hdr, hdrLen, 0);
    send(client_fd, body.data(), body.size(), 0);
    close(client_fd);
}