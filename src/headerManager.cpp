#include "include/headerManager.h"
#include <string>
#include <cctype>

using namespace std;

static string addHeaderBeforeEnding(const string &header, const string &toAdd)
{
    size_t pos = header.find("\r\n\r\n");
    if (pos == string::npos)
        return header; // invalid header, just return as is
    return header.substr(0, pos + 2) + toAdd + header.substr(pos + 2);
}

string headerManager(const string &header) // expects a header and just adds extra stuff to it
{
    string modifiedHeader = header;

    // start off by validating the header, checking common headers and /r/n/r/n ending
    if (modifiedHeader.size() < 16) // minimum valid header size
        return "invalid";
    if (modifiedHeader.substr(0, 5) != "HTTP/") // must start with HTTP/
        return "invalid";
    if (modifiedHeader.find("\r\n\r\n") == string::npos) // must end with \r\n\r\n
        return "invalid";
    // good enough atp

    // add common headers
    modifiedHeader = addHeaderBeforeEnding(modifiedHeader, "Server: faucet/1.0\r\n");

    // basic security headers, while remaining flexible
    modifiedHeader = addHeaderBeforeEnding(modifiedHeader, "X-Content-Type-Options: nosniff\r\n");
    modifiedHeader = addHeaderBeforeEnding(modifiedHeader, "Referrer-Policy: no-referrer\r\n");

    // return the finished header
    return modifiedHeader;
}