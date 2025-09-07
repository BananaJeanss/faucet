#include "include/return404.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include <unistd.h>
#include <cstring>
#include "include/perMinute404.h"

#include "include/returnErrorPage.h"

extern void return404(int client_fd, 
    const std::string 
    &siteDir, 
    const std::string &Page404, 
    const std::string &contactEmail,
    const std::string &ip)
{
    // if custom 404 page is set, try to serve it
    add404PMentry(ip);
    if (!Page404.empty())
    {
        std::string fullPath = siteDir.empty() ? Page404 : (siteDir + "/" + Page404);
        int opened_fd = open(fullPath.c_str(), O_RDONLY);
        if (opened_fd != -1)
        {
            struct stat st{};
            if (fstat(opened_fd, &st) == 0 && S_ISREG(st.st_mode))
            {
                // send file
                const char *ctype = "text/html"; // assume html for custom 404

                char header[256];
                int header_len = snprintf(header, sizeof(header),
                                          "HTTP/1.1 404 Not Found\r\n"
                                          "Content-Length: %lld\r\n"
                                          "Content-Type: %s\r\n"
                                          "Connection: close\r\n"
                                          "\r\n",
                                          (long long)st.st_size, ctype);
                if (header_len >= 0 && header_len < (int)sizeof(header))
                {
                    send(client_fd, header, header_len, 0);

                    // send full file
                    off_t offset = 0;
                    while (offset < st.st_size)
                    {
                        ssize_t sent = sendfile(client_fd, opened_fd, &offset, st.st_size - offset);
                        if (sent <= 0)
                            break; // error or EOF
                    }
                    close(opened_fd);
                    close(client_fd);
                    return;
                }
            }
            close(opened_fd); // not a regular file, close
        }
        else
        {
            // could not open custom 404 page, fallback to basic 404
            perror("open");
            returnErrorPage(client_fd, 404, contactEmail);
        }
    }
    else
    {
        // no custom 404 page set, return returnErrorPage
        returnErrorPage(client_fd, 404, contactEmail); // returnErrorPage handles closing client_fd yadayada
    }
}