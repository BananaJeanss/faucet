#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <signal.h>
#include <cstdlib>
#include <errno.h>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <cctype>

#include "include/loadConfig.h"
#include "include/return404.h"
#include "include/returnDirListing.h"
#include "include/contentTypes.h"
#include <vector>
#include "include/returnErrorPage.h"

using namespace std;

static volatile sig_atomic_t keepRunning = 1;

static void sig_handler(int)
{
    keepRunning = 0;
}

// config values
string loadEnv;
int port = 8080;
string siteDir = "public";
string Page404 = ""; // relative to siteDir, empty for none
bool useDirListing = false;
int requestRateLimit = 10; // requests/second per IP, 0 for none
string contactEmail = "";
string authCredentials = ""; // user:password for basic auth, empty for none

string authUser = "";
string authPass = "";
bool authEnabled = false;
std::string expectedAuthValue; // basic base64

// ip ratelimit struct
struct IpRateLimit
{
    string ip;
    int requestCount;
    time_t lastRequestTime;
};

std::vector<IpRateLimit> ipRateLimits;

// base64 encoder for auth
static std::string base64Encode(const std::string &in)
{
    static const char *tbl = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve(((in.size() + 2) / 3) * 4);
    size_t i = 0;
    while (i + 2 < in.size())
    {
        unsigned int n = (unsigned char)in[i] << 16 | (unsigned char)in[i + 1] << 8 | (unsigned char)in[i + 2];
        out.push_back(tbl[(n >> 18) & 63]);
        out.push_back(tbl[(n >> 12) & 63]);
        out.push_back(tbl[(n >> 6) & 63]);
        out.push_back(tbl[n & 63]);
        i += 3;
    }
    if (i + 1 < in.size())
    {
        unsigned int n = (unsigned char)in[i] << 16 | (unsigned char)in[i + 1] << 8;
        out.push_back(tbl[(n >> 18) & 63]);
        out.push_back(tbl[(n >> 12) & 63]);
        out.push_back(tbl[(n >> 6) & 63]);
        out.push_back('=');
    }
    else if (i < in.size())
    {
        unsigned int n = (unsigned char)in[i] << 16;
        out.push_back(tbl[(n >> 18) & 63]);
        out.push_back(tbl[(n >> 12) & 63]);
        out.push_back('=');
        out.push_back('=');
    }
    return out;
}

int main(int argc, char *argv[])
{
    struct sigaction sa{};
    sa.sa_handler = sig_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, nullptr);

    printf("faucet http server\n");

    // load config
    int confResult = loadConfig(port,
                                siteDir,
                                Page404,
                                useDirListing,
                                requestRateLimit,
                                contactEmail,
                                authCredentials);
    if (confResult == 1)
    {
        printf("Failed to load config, check the .env file.\n");
        printf("Continuing with defaults...\n");
    }
    if (confResult == 2)
    {
        printf("No config found, created default .env file.\n");
        printf("Continuing with defaults...\n");
    }

    // convert authCredentials to authuser/authpass
    if (!authCredentials.empty())
    {
        size_t colonPos = authCredentials.find(':');
        if (colonPos != std::string::npos)
        {
            authUser = authCredentials.substr(0, colonPos);
            authPass = authCredentials.substr(colonPos + 1);
            if (!authUser.empty() && !authPass.empty())
            {
                authEnabled = true;
                expectedAuthValue = std::string("Basic ") + base64Encode(authUser + ":" + authPass);
                printf("HTTP Auth Enabled (user: %s)\n", authUser.c_str());
            }
        }
    } else {
        authEnabled = false; // false anyways but whatever
    }

    // normalize siteDir
    auto normalizeDir = [](std::string d)
    {
        // remove all leading slashes
        while (!d.empty() && d.front() == '/')
            d.erase(0, 1);
        // remove all trailing slashes
        while (!d.empty() && d.back() == '/')
            d.pop_back();
        return d;
    };
    siteDir = normalizeDir(siteDir);

    // loop through args
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--port") == 0 && i + 1 < argc)
        {
            // make sure port is no larger than 65535
            int p = atoi(argv[i + 1]);
            if (p < 1 || p > 65535)
            {
                printf("Invalid port number: %s\n", argv[i + 1]);
                return 1;
            }
            port = atoi(argv[i + 1]);
            i++;
        }
        else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
        {
            printf("Usage: %s [--port <port>] [--help]\n", argv[0]);
            return 0;
        }
        else
        {
            printf("Unknown argument: %s\n", argv[i]);
            printf("Usage: %s [--port <port>] [--help]\n", argv[0]);
            return 1;
        }
    }

    // create the socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1)
    {
        perror("socket");
        return 1;
    }
    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // struct for the bind
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    // bind
    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("bind");
        return 1;
    }

    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr.sin_addr, ip, sizeof(ip));
    printf("Listening on %s:%d, serving from %s\n", ip, ntohs(addr.sin_port), siteDir.c_str());
    fflush(stdout);

    // listen
    if (listen(sock, 10) < 0)
    {
        perror("listen");
        return 1;
    }

    printf("---\n");

    // loop accept
    for (;;)
    {
        if (!keepRunning)
            break;

        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(sock, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0)
        {
            if (errno == EINTR)
            {
                // interrupted by signal, check flag.
                if (!keepRunning)
                    break;
                continue;
            }
            perror("accept");
            break;
        }

        char clientIp[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, clientIp, sizeof(clientIp));

        // log request /w timestamp
        auto t = time(nullptr);
        auto tm = *localtime(&t);
        char timebuf[32];
        strftime(timebuf, sizeof(timebuf), "%d-%m-%Y %H:%M:%S", &tm);

        printf("[%s] Received request from %s:%d\n", timebuf, clientIp, ntohs(client_addr.sin_port));
        fflush(stdout);

        if (requestRateLimit > 0)
        {
            // check ip rate limit
            time_t now = time(nullptr);
            bool found = false;
            for (auto &entry : ipRateLimits)
            {
                if (entry.ip == clientIp)
                {
                    found = true;
                    if (now == entry.lastRequestTime)
                    {
                        entry.requestCount++;
                    }
                    else
                    {
                        entry.requestCount = 1;
                        entry.lastRequestTime = now;
                    }

                    if (entry.requestCount > requestRateLimit)
                    {
                        // over limit, send 429 and close
                        returnErrorPage(client_fd, 429, contactEmail);
                        printf("[%s] Rate limit exceeded for %s\n", timebuf, clientIp);
                        fflush(stdout);
                        found = true;
                        break;
                    }
                    break;
                }
            }
            if (!found)
            {
                IpRateLimit newEntry{};
                newEntry.ip = clientIp;
                newEntry.requestCount = 1;
                newEntry.lastRequestTime = now;
                ipRateLimits.push_back(newEntry);
            }
        }

        // read headers/request
        char buffer[4096];
        ssize_t used = 0;
        const char *eolmark = "\r\n\r\n"; // until eol
        bool closedEarly = false;
        while (used < (ssize_t)sizeof(buffer) - 1)
        {
            ssize_t recvd = recv(client_fd, buffer + used, sizeof(buffer) - 1 - used, 0);
            if (recvd <= 0)
            {
                // peer closed or error, abort processing this request safely
                close(client_fd);
                closedEarly = true;
                break;
            }
            used += recvd;
            buffer[used] = 0; // null terminate for strstr

            if (strstr(buffer, eolmark))
                break; // got all headers
        }

        if (closedEarly)
            continue;

        // if header too large/malformed, close
        if (!strstr(buffer, eolmark))
        {
            returnErrorPage(client_fd, 400, contactEmail);
            continue;
        }

        // expect GET path HTTP/1.1
        if (strncmp(buffer, "GET ", 4) != 0)
        {
            // unsupported method
            close(client_fd);
            continue;
        }

        // if auth enabled, check for correct auth header
        if (authEnabled)
        {
            // Extract headers as string
            const char *hdrEnd = strstr(buffer, "\r\n\r\n");
            size_t headerLen = hdrEnd ? (size_t)(hdrEnd - buffer) : (size_t)used;
            std::string headers(buffer, headerLen);

            bool authOk = false;
            size_t lineStart = 0;
            while (lineStart < headers.size())
            {
                size_t lineEnd = headers.find("\r\n", lineStart);
                if (lineEnd == std::string::npos)
                    lineEnd = headers.size();
                std::string line = headers.substr(lineStart, lineEnd - lineStart);
                // case-insensitive check for Authorization:
                if (line.size() >= 14) // minimum length
                {
                    bool isAuth = true;
                    const std::string key = "authorization:"; // lower
                    for (size_t k = 0; k < key.size() && k < line.size(); ++k)
                    {
                        if (std::tolower((unsigned char)line[k]) != key[k])
                        {
                            isAuth = false;
                            break;
                        }
                    }
                    if (isAuth)
                    {
                        // get value after colon
                        size_t colon = line.find(':');
                        if (colon != std::string::npos)
                        {
                            std::string value = line.substr(colon + 1);
                            // trim leading spaces
                            while (!value.empty() && (value[0] == ' ' || value[0] == '\t'))
                                value.erase(0, 1);
                            if (value == expectedAuthValue)
                                printf("  Auth success for %s\n", clientIp);
                                authOk = true;
                        }
                        break;
                    }
                }
                if (lineEnd == headers.size())
                    break;
                lineStart = lineEnd + 2; // skip CRLF
            }
            if (!authOk)
            {
                const char *hdr = "HTTP/1.1 401 Unauthorized\r\n"
                                  "WWW-Authenticate: Basic realm=\"faucet\"\r\n"
                                  "Cache-Control: no-store\r\n"
                                  "Content-Length: 0\r\n"
                                  "Connection: close\r\n"
                                  "\r\n";
                send(client_fd, hdr, strlen(hdr), 0);
                close(client_fd);
                continue;
            }
        }

        char *path_start = buffer + 4; // after GET
        char *path_end = strchr(path_start, ' ');
        if (!path_end) // malformed
        {
            close(client_fd);
            continue;
        }
        *path_end = 0;

        // map / to index.html if no file specified
        bool userSetFile = true;
        if (strcmp(path_start, "/") == 0)
        {
            path_start = (char *)"/index.html";
            userSetFile = false;
        }

        // reject .. for simple security
        if (strstr(path_start, ".."))
        {
            returnErrorPage(client_fd, 400, contactEmail);
            continue;
        }

        // strip leading slash for filesystem open
        const char *rel_path = (path_start[0] == '/') ? path_start + 1 : path_start;

        // return 418 for /imateapot418 if file/dir does not exist
        if (strcmp(path_start, "/imateapot418") == 0)
        {
            const std::string fullPath = siteDir.empty() ? "imateapot418" : (siteDir + "/imateapot418");
            struct stat st{};
            if (stat(fullPath.c_str(), &st) != 0)
            {
                returnErrorPage(client_fd, 418, contactEmail);
                continue;
            }
        }

        // handle explicit directory requests ending with '/'
        if (path_start[strlen(path_start) - 1] == '/')
        {
            // rel_path currently ends with '/', trim for filesystem path
            std::string dirRel = rel_path;
            while (!dirRel.empty() && dirRel.back() == '/')
                dirRel.pop_back();
            std::string dirFull = siteDir.empty() ? dirRel : (siteDir + "/" + dirRel);

            struct stat dst{};
            if (stat(dirFull.c_str(), &dst) == 0 && S_ISDIR(dst.st_mode))
            {
                // try common index files
                const char *indices[] = {"index.html", "index.htm"};
                bool servedIndex = false;
                for (const char *idx : indices)
                {
                    std::string idxFull = dirFull + "/" + idx;
                    int fd = open(idxFull.c_str(), O_RDONLY);
                    if (fd != -1)
                    {
                        struct stat ist{};
                        if (fstat(fd, &ist) == 0 && S_ISREG(ist.st_mode))
                        {
                            const char *ctype = guessContentType(idxFull.c_str());
                            char header[256];
                            int header_len = snprintf(header, sizeof(header),
                                                      "HTTP/1.1 200 OK\r\n"
                                                      "Content-Length: %lld\r\n"
                                                      "Content-Type: %s\r\n"
                                                      "Connection: close\r\n"
                                                      "\r\n",
                                                      (long long)ist.st_size, ctype);
                            if (header_len > 0 && header_len < (int)sizeof(header))
                            {
                                send(client_fd, header, header_len, 0);
                                off_t off = 0;
                                while (off < ist.st_size)
                                {
                                    ssize_t s = sendfile(client_fd, fd, &off, ist.st_size - off);
                                    if (s <= 0)
                                        break;
                                }
                            }
                            close(fd);
                            close(client_fd);
                            servedIndex = true;
                            break;
                        }
                        close(fd);
                    }
                }
                if (servedIndex)
                    continue;

                // no index file; directory listing or 404
                if (useDirListing)
                {
                    returnDirListing(client_fd, siteDir, dirRel, Page404, contactEmail);
                }
                else
                {
                    return404(client_fd, siteDir, Page404, contactEmail);
                }
                continue;
            }
        }

        // build full path inside of siteDir
        std::string fullPath = siteDir.empty() ? rel_path : (siteDir + "/" + rel_path);

        struct stat pathStat{};
        if (stat(fullPath.c_str(), &pathStat) == 0 && S_ISDIR(pathStat.st_mode))
        {
            bool hasTrailingSlash = (path_start[strlen(path_start) - 1] == '/');
            if (!hasTrailingSlash)
            {
                // send 301 redirect to canonical slash form
                std::string loc = std::string(path_start) + "/";
                char hdr[512];
                int hdrLen = snprintf(hdr, sizeof(hdr),
                                      "HTTP/1.1 301 Moved Permanently\r\n"
                                      "Location: %s\r\n"
                                      "Content-Length: 0\r\n"
                                      "Connection: close\r\n"
                                      "\r\n",
                                      loc.c_str());
                if (hdrLen > 0 && hdrLen < (int)sizeof(hdr))
                    send(client_fd, hdr, hdrLen, 0);
                close(client_fd);
                continue;
            }

            // try index files
            const char *indices[] = {"index.html", "index.htm"};
            bool servedIndex = false;
            for (const char *idx : indices)
            {
                std::string idxFull = fullPath + "/" + idx;
                int fd = open(idxFull.c_str(), O_RDONLY);
                if (fd != -1)
                {
                    struct stat ist{};
                    if (fstat(fd, &ist) == 0 && S_ISREG(ist.st_mode))
                    {
                        const char *ctype = guessContentType(idxFull.c_str());
                        char header[256];
                        int header_len = snprintf(header, sizeof(header),
                                                  "HTTP/1.1 200 OK\r\n"
                                                  "Content-Length: %lld\r\n"
                                                  "Content-Type: %s\r\n"
                                                  "Connection: close\r\n"
                                                  "\r\n",
                                                  (long long)ist.st_size, ctype);
                        if (header_len > 0 && header_len < (int)sizeof(header))
                        {
                            send(client_fd, header, header_len, 0);
                            off_t off = 0;
                            while (off < ist.st_size)
                            {
                                ssize_t s = sendfile(client_fd, fd, &off, ist.st_size - off);
                                if (s <= 0)
                                    break;
                            }
                        }
                        close(fd);
                        close(client_fd);
                        servedIndex = true;
                        break;
                    }
                    close(fd);
                }
            }
            if (servedIndex)
                continue;

            // no index,  directory listing or 404
            if (useDirListing)
            {
                // rel_path currently without leading slash already
                returnDirListing(client_fd, siteDir, rel_path, Page404, contactEmail);
            }
            else
            {
                return404(client_fd, siteDir, Page404, contactEmail);
            }
            continue;
        }
        int opened_fd = open(fullPath.c_str(), O_RDONLY);
        if (opened_fd == -1) // file not found
        {
            // if dirlisting enabled and user did not specify file, show dir listing
            if (useDirListing && !userSetFile)
            {
                returnDirListing(client_fd, siteDir, rel_path, Page404, contactEmail);
                continue;
            }
            else
            {
                return404(client_fd, siteDir, Page404, contactEmail);
                continue;
            }
        }
        struct stat st{};
        if (fstat(opened_fd, &st) < 0 || !S_ISREG(st.st_mode))
        {
            return404(client_fd, siteDir, Page404, contactEmail);
            close(opened_fd); // not a regular file, close
            continue;
        }

        // send file
        // guess content type based on extension for proper loading in browsers
        const char *ctype = guessContentType(fullPath.c_str());

        char header[256];
        int header_len = snprintf(header, sizeof(header),
                                  "HTTP/1.1 200 OK\r\n"
                                  "Content-Length: %lld\r\n"
                                  "Content-Type: %s\r\n"
                                  "Connection: close\r\n"
                                  "\r\n",
                                  (long long)st.st_size, ctype);
        if (header_len < 0 || header_len >= (int)sizeof(header))
        {
            close(opened_fd);
            close(client_fd);
            continue;
        }
        send(client_fd, header, header_len, 0);

        // send full file
        off_t offset = 0;
        while (offset < st.st_size)
        {
            ssize_t sent = sendfile(client_fd, opened_fd, &offset, st.st_size - offset);
            if (sent <= 0)
                break; // error or EOF
        }

        // close connections
        close(opened_fd);
        close(client_fd);
    }

    printf("Shutting down...\n");
    close(sock);
    return 0;
}