#include "include/returnErrorPage.h"
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>

using namespace std;

void returnErrorPage(int client_fd, int errorType, string contactMail)
{
    // build html body
    const char *ctype = "text/html; charset=utf-8";
    static const char *pageTemplate =
        "<!DOCTYPE html><html><head><meta charset=\"utf-8\"><title>{ErrorCode} {ErrorTextLite}</title>"
        "<style>body{font-family:sans-serif;text-align:center;margin-top:50px}h1{font-size:48px}p{font-size:24px}</style>"
        "</head><body><h1>{ErrorCode}</h1><hr><p>{ErrorText}</p><a href=\"/\">Return to Home</a></body></html>";

    string errorText;
    switch (errorType)
    {
    case 400:
        errorText = "Bad Request";
        break;
    case 401:
        errorText = "Unauthorized";
        break;
    case 403:
        errorText = "Forbidden";
        break;
    case 404:
        errorText = "Not Found";
        break;
    case 500:
        errorText = "Internal Server Error";
        break;
    case 501:
        errorText = "Not Implemented";
        break;
    case 503:
        errorText = "Service Unavailable";
        break;
    case 418:
        errorText = "I'm a teapot";
        break; // haha funni
    case 429:
        errorText = "Too Many Requests";
        break;
    default:
        errorText = "Unknown Error";
        break;
    }

    string body = pageTemplate;
    size_t pos;
    while ((pos = body.find("{ErrorCode}")) != string::npos)
        body.replace(pos, 11, to_string(errorType));
    while ((pos = body.find("{ErrorText}")) != string::npos)
        body.replace(pos, 11, errorText + (contactMail.empty() ? "" : (".<br>Contact: " + contactMail)));
    while ((pos = body.find("{ErrorTextLite}")) != string::npos)
        body.replace(pos, 15, errorText); // just errortext without br and contact

    // HTTP header
    string header;
    if (errorType == 401)
    {
        header = "HTTP/1.1 401 Unauthorized\r\n";
        header += "WWW-Authenticate: Basic realm=\"faucet\"\r\n";
        header += "Cache-Control: no-store\r\n";
        header += "Content-Type: ";
        header += ctype;
        header += "\r\nContent-Length: ";
        header += to_string(body.size());
        header += "\r\nConnection: close\r\n\r\n";
    }
    else
    {
        header = "HTTP/1.1 " + to_string(errorType) + " " + errorText + "\r\n";
        header += "Content-Type: ";
        header += ctype;
        header += "\r\nContent-Length: ";
        header += to_string(body.size());
        header += "\r\nConnection: close\r\n\r\n";
    }

    string response = header + body;

    send(client_fd, response.c_str(), response.size(), 0);
    close(client_fd);
}