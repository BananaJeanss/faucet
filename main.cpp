#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main()
{
    printf("faucet http server\n");
    printf("---\n");

    // create the socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    // struct for the bind
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = INADDR_ANY;

    // bind
    bind(sock, (struct sockaddr *)&addr, sizeof(addr));

    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr.sin_addr, ip, sizeof(ip));
    printf("\nListening on %s:%d\n", ip, ntohs(addr.sin_port));
    fflush(stdout);

    // listen
    listen(sock, 10);

    // accept
    int client_fd = accept(sock, 0, 0);

    char buffer[256] = {0};
    recv(client_fd, buffer, 256, 0);

    // GET /index.html .....

    char *f = buffer + 5; // skip first 5 chars for index.html
    *strchr(f, ' ') = 0;

    // open requested file
    int opened_fd = open(f, O_RDONLY);
    // send file
    sendfile(client_fd, opened_fd, 0, 256);

    // close connections/sock
    close(opened_fd);
    close(client_fd);
    close(sock);
}