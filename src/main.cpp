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

using namespace std;

#include "include/loadConfig.h"
#include "include/return404.h"

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

int main(int argc, char *argv[])
{
    struct sigaction sa{};
    sa.sa_handler = sig_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, nullptr);

    printf("faucet http server\n");

    // load config
    int confResult = loadConfig(port, siteDir, Page404);
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
        printf("Received request from %s:%d\n", clientIp, ntohs(client_addr.sin_port));
        fflush(stdout);

        // read headers/request
        char buffer[4096];
        ssize_t used = 0;
        const char *eolmark = "\r\n\r\n"; // until eol
        while (used < (ssize_t)sizeof(buffer) - 1)
        {
            ssize_t recvd = recv(client_fd, buffer + used, sizeof(buffer) - 1 - used, 0);
            if (recvd <= 0)
            {
                // error or closed
                close(client_fd);
            }
            used += recvd;
            buffer[used] = 0; // null terminate for strstr

            if (strstr(buffer, eolmark))
                break; // got all headers
        }

        // if header too large/malformed, close
        if (!strstr(buffer, eolmark))
        {
            const char *hdr = "HTTP/1.1 400 Bad Request\r\n\r\n";
            send(client_fd, hdr, strlen(hdr), 0);
            close(client_fd);
            return 0;
        }

        // expect GET path HTTP/1.1
        if (strncmp(buffer, "GET ", 4) != 0)
        {
            // unsupported method
            close(client_fd);
            return 0;
        }

        char *path_start = buffer + 4; // after GET
        char *path_end = strchr(path_start, ' ');
        if (!path_end) // malformed
        {
            close(client_fd);
            close(sock);
            return 0;
        }
        *path_end = 0;

        // map / to index.html
        if (strcmp(path_start, "/") == 0)
        {
            path_start = (char *)"/index.html";
        }

        // reject .. for simple security
        if (strstr(path_start, ".."))
        {
            const char *hdr = "HTTP/1.1 400 Bad Request\r\n\r\n";
            send(client_fd, hdr, strlen(hdr), 0);
            close(client_fd);
            close(sock);
            return 0;
        }

        // strip leading slash for filesystem open
        const char *rel_path = (path_start[0] == '/') ? path_start + 1 : path_start;

        // build full path inside of siteDir
        std::string fullPath = siteDir.empty() ? rel_path : (siteDir + "/" + rel_path);
        int opened_fd = open(fullPath.c_str(), O_RDONLY);
        if (opened_fd == -1)
        {
            return404(client_fd, siteDir, Page404);
            continue;
        }
        struct stat st{};
        if (fstat(opened_fd, &st) < 0 || !S_ISREG(st.st_mode))
        {
            return404(client_fd, siteDir, Page404);
            close(opened_fd); // not a regular file, close
            continue;
        }

        // send file
        // guess content type based on extension for proper loading in browsers
        auto guessContentType = [](const char *path) -> const char *
        {
            const char *dot = strrchr(path, '.');
            if (!dot)
                return "application/octet-stream";
            if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0)
                return "text/html";
            if (strcmp(dot, ".css") == 0)
                return "text/css";
            if (strcmp(dot, ".js") == 0)
                return "text/javascript";
            if (strcmp(dot, ".json") == 0)
                return "application/json";
            if (strcmp(dot, ".png") == 0)
                return "image/png";
            if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0)
                return "image/jpeg";
            if (strcmp(dot, ".gif") == 0)
                return "image/gif";
            return "application/octet-stream";
        };

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