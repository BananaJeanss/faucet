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

int main()
{
    printf("faucet http server\n");
    printf("---\n");

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
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = INADDR_ANY;

    // bind
    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("bind");
        return 1;
    }

    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr.sin_addr, ip, sizeof(ip));
    printf("\nListening on %s:%d\n", ip, ntohs(addr.sin_port));
    fflush(stdout);

    // listen
    if (listen(sock, 10) < 0)
    {
        perror("listen");
        return 1;
    }

    // loop accept
    for (;;)
    {
        int client_fd = accept(sock, 0, 0);

        char buffer[256] = {0};
        ssize_t n = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (n <= 0)
        {
            perror("recv");
            close(client_fd);
            close(sock);
            return 1;
        }
        buffer[n] = 0;

        int requestIp = ntohl(addr.sin_addr.s_addr);

        printf("Received request from %d.%d.%d.%d\n",
               (requestIp >> 24) & 0xFF,
               (requestIp >> 16) & 0xFF,
               (requestIp >> 8) & 0xFF,
               requestIp & 0xFF);

        // expect GET path HTTP/1.1
        if (strncmp(buffer, "GET ", 4) != 0)
        {
            // unsupported method
            close(client_fd);
            close(sock);
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
        const char *fs_path = (path_start[0] == '/') ? path_start + 1 : path_start;

        int opened_fd = open(fs_path, O_RDONLY);
        if (opened_fd == -1)
        {
            const char *hdr = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
            send(client_fd, hdr, strlen(hdr), 0);
            close(client_fd);
            continue;
        }

        struct stat st{};
        if (fstat(opened_fd, &st) < 0 || !S_ISREG(st.st_mode))
        {
            const char *hdr = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
            send(client_fd, hdr, strlen(hdr), 0);
            close(opened_fd);
            close(client_fd);
            continue;
        }

        // send file
        char header[256];
        int header_len = snprintf(header, sizeof(header),
                                  "HTTP/1.1 200 OK\r\n"
                                  "Content-Length: %lld\r\n"
                                  "Content-Type: text/html\r\n"
                                  "Connection: close\r\n"
                                  "\r\n",
                                  (long long)st.st_size);
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

    close(sock);
}