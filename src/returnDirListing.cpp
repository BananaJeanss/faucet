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
#include <cctype>
#include <sstream>

using namespace std;

static const char *folderSvg =
    "<svg xmlns=\"http://www.w3.org/2000/svg\" "
    "width=\"18\" height=\"18\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"%23e0e0e0\" stroke-width=\"2\" "
    "stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"lucide lucide-folder-icon lucide-folder\">"
    "<path d=\"M20 20a2 2 0 0 0 2-2V8a2 2 0 0 0-2-2h-7.9a2 2 0 0 1-1.69-.9L9.6 3.9A2 2 0 0 0 7.93 3H4a2 2 0 0 0-2 2v13a2 2 0 0 0 2 2Z\"/></svg>";

static const char *fileSvg =
    "<svg xmlns=\"http://www.w3.org/2000/svg\" "
    "width=\"18\" height=\"18\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"%23e0e0e0\" stroke-width=\"2\" "
    "stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"lucide lucide-file-icon lucide-file\">"
    "<path d=\"M15 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V7Z\"/>"
    "<path d=\"M14 2v4a2 2 0 0 0 2 2h4\"/></svg>";

static const char *homeSvg =
    "<svg xmlns=\"http://www.w3.org/2000/svg\" "
    "width=\"18\" height=\"18\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"%23e0e0e0\" stroke-width=\"2\" "
    "stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"lucide lucide-house-icon lucide-house\">"
    "<path d=\"M15 21v-8a1 1 0 0 0-1-1h-4a1 1 0 0 0-1 1v8\"/>"
    "<path d=\"M3 10a2 2 0 0 1 .709-1.528l7-5.999a2 2 0 0 1 2.582 0l7 5.999A2 2 0 0 1 21 10v9a2 2 0 0 1-2 2H5a2 2 0 0 1-2 2z\"/></svg>";

static const char *upFolderSvg =
    "<svg xmlns=\"http://www.w3.org/2000/svg\" "
    "width=\"18\" height=\"18\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"%23e0e0e0\" stroke-width=\"2\" "
    "stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"lucide lucide-folder-up-icon lucide-folder-up\">"
    "<path d=\"M20 20a2 2 0 0 0 2-2V8a2 2 0 0 0-2-2h-7.9a2 2 0 0 1-1.69-.9L9.6 3.9A2 2 0 0 0 7.93 3H4a2 2 0 0 0-2 2v13a2 2 0 0 0 2 2Z\"/>"
    "<path d=\"M12 10v6\"/>"
    "<path d=\"m9 13 3-3 3 3\"/></svg>";

static const char *filmSvg =
    "<svg xmlns=\"http://www.w3.org/2000/svg\" "
    "width=\"18\" height=\"18\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"%23e0e0e0\" stroke-width=\"2\" "
    "stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"lucide lucide-film-icon lucide-film\">"
    "<rect width=\"18\" height=\"18\" x=\"3\" y=\"3\" rx=\"2\"/>"
    "<path d=\"M7 3v18\"/>"
    "<path d=\"M3 7.5h4\"/>"
    "<path d=\"M3 12h18\"/>"
    "<path d=\"M3 16.5h4\"/>"
    "<path d=\"M17 3v18\"/>"
    "<path d=\"M17 7.5h4\"/>"
    "<path d=\"M17 16.5h4\"/></svg>";

static const char *imageSvg =
    "<svg xmlns=\"http://www.w3.org/2000/svg\" "
    "width=\"18\" height=\"18\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"%23e0e0e0\" stroke-width=\"2\" "
    "stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"lucide lucide-file-image-icon lucide-file-image\">"
    "<path d=\"M15 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V7Z\"/>"
    "<path d=\"M14 2v4a2 2 0 0 0 2 2h4\"/>"
    "<circle cx=\"10\" cy=\"12\" r=\"2\"/>"
    "<path d=\"m20 17-1.296-1.296a2.41 2.41 0 0 0-3.408 0L9 22\"/></svg>";

static const char *textfileSvg =
    "<svg xmlns=\"http://www.w3.org/2000/svg\" "
    "width=\"18\" height=\"18\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"%23e0e0e0\" stroke-width=\"2\" "
    "stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"lucide lucide-file-text-icon lucide-file-text\">"
    "<path d=\"M15 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V7Z\"/>"
    "<path d=\"M14 2v4a2 2 0 0 0 2 2h4\"/>"
    "<path d=\"M10 9H8\"/>"
    "<path d=\"M16 13H8\"/>"
    "<path d=\"M16 17H8\"/></svg>";

static const char *activitySvg =
    "<svg xmlns=\"http://www.w3.org/2000/svg\" "
    "width=\"18\" height=\"18\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"%23e0e0e0\" stroke-width=\"2\" "
    "stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"lucide lucide-activity-icon lucide-activity\">"
    "<path d=\"M22 12h-2.48a2 2 0 0 0-1.93 1.46l-2.35 8.36a.25.25 0 0 1-.48 0L9.24 2.18a.25.25 0 0 0-.48 0l-2.35 8.36A2 2 0 0 1 4.49 12H2\"/></svg>";

static const char *codexmlSvg =
    "<svg xmlns=\"http://www.w3.org/2000/svg\" "
    "width=\"18\" height=\"18\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"%23e0e0e0\" stroke-width=\"2\" "
    "stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"lucide lucide-code-xml-icon lucide-code-xml\">"
    "<path d=\"m18 16 4-4-4-4\"/>"
    "<path d=\"m6 8-4 4 4 4\"/>"
    "<path d=\"m14.5 4-5 16\"/>"
    "</svg>";

static const char *musicSvg =
    "<svg xmlns=\"http://www.w3.org/2000/svg\" "
    "width=\"18\" height=\"18\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"%23e0e0e0\" stroke-width=\"2\" "
    "stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"lucide lucide-music-icon lucide-music\">"
    "<path d=\"M9 18V5l12-2v13\"/>"
    "<circle cx=\"6\" cy=\"18\" r=\"3\"/>"
    "<circle cx=\"18\" cy=\"16\" r=\"3\"/>"
    "</svg>";

static const std::string styling =
    "<style>"
    "body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; margin: 20px; background-color: #2d2d2d; color: #e0e0e0; }"
    "h1 { font-size: 24px; color: #f0f0f0; }"
    "table { width: 100%; border-collapse: collapse; }"
    "footer p { font-size: 12px; color: #888; margin-top: 20px; }"
    "th, td { padding: 8px 12px; border-bottom: 1px solid #444; text-align: left; }"
    "th { background-color: #3a3a3a; color: #f0f0f0; }"
    "a { text-decoration: none; color: #6db3f2; }"
    "a:hover { text-decoration: underline; color: #85c1ff; }"
    "hr { border: none; border-top: 1px solid #444; }"
    ".file { display: inline-block; }"
    ".directory { display: inline-block; } "
    ".home { display: inline-block; } "
    ".directory:before { content: url('data:image/svg+xml;utf8," +
    std::string(folderSvg) + "'); vertical-align: middle; margin-right: 8px; color: #f0f0f0; }"
                             ".file:before { content: url('data:image/svg+xml;utf8," +
    std::string(fileSvg) + "'); vertical-align: middle; margin-right: 8px; color: #f0f0f0; }"
                           ".home:before { content: url('data:image/svg+xml;utf8," +
    std::string(homeSvg) + "'); vertical-align: middle; margin-right: 8px; color: #f0f0f0; }"
                            ".upfolder:before { content: url('data:image/svg+xml;utf8," +
    std::string(upFolderSvg) + "'); vertical-align: middle; margin-right: 8px; color: #f0f0f0; }"
                            ".film:before { content: url('data:image/svg+xml;utf8," +
    std::string(filmSvg) + "'); vertical-align: middle; margin-right: 8px; color: #f0f0f0; }"
                            ".image:before { content: url('data:image/svg+xml;utf8," +
    std::string(imageSvg) + "'); vertical-align: middle; margin-right: 8px; color: #f0f0f0; }"
                            ".textfile:before { content: url('data:image/svg+xml;utf8," +
    std::string(textfileSvg) + "'); vertical-align: middle; margin-right: 8px; color: #f0f0f0; }"
                            ".activity:before { content: url('data:image/svg+xml;utf8," +
    std::string(activitySvg) + "'); vertical-align: middle; margin-right: 8px; color: #f0f0f0; }"
                            ".codexml:before { content: url('data:image/svg+xml;utf8," +
    std::string(codexmlSvg) + "'); vertical-align: middle; margin-right: 8px; color: #f0f0f0; }"
                            ".music:before { content: url('data:image/svg+xml;utf8," +
    std::string(musicSvg) + "'); vertical-align: middle; margin-right: 8px; color: #f0f0f0; }"
                           "</style>";

static const std::string pageHeader =
    "<!DOCTYPE html><html><head><meta charset=\"utf-8\"><title>Index of {Dir}</title>" + styling +
    "</head><body><h1>Index of {Dir}</h1><table>";

static const char *pageFooter = "</table></body>"
    "<footer><p>Powered by faucet</p></footer>"
    "</html>";

static std::string getFileTypeClass(const std::string &filename) {
    // determine file type by extension for icon
    size_t dotPos = filename.rfind('.');
    if (dotPos == std::string::npos || dotPos == filename.length() - 1)
        return "file"; // no extension

    std::string ext = filename.substr(dotPos + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return std::tolower(c); });

    if (ext == "mp4" || ext == "mkv" || ext == "avi" || ext == "mov" || ext == "wmv" || ext == "flv" || ext == "webm")
        return "film";
    if (ext == "jpg" || ext == "jpeg" || ext == "png" || ext == "gif" || ext == "bmp" || ext == "svg" || ext == "webp" || ext == "tiff")
        return "image";
    if (ext == "txt" || ext == "md" || ext == "log" || ext == "csv" || ext == "json" || ext == "xml" || ext == "ini" || ext == "cfg")
        return "textfile";
    if (ext == "html" || ext == "htm" || ext == "css" || ext == "js" || ext == "cpp" || ext == "h" || ext == "hpp" || ext == "c" || ext == "java" || ext == "py" || ext == "rb" || ext == "go")
        return "codexml";
    if (ext == "mp3" || ext == "wav" || ext == "flac" || ext == "aac" || ext == "ogg" || ext == "m4a")
        return "music";
    if (ext == "log")
        return "activity";
    return "file"; // default
}

void returnDirListing(int client_fd,
                      const std::string &siteDir,
                      const std::string &relPath,
                      const std::string &Page404,
                      const std::string &contactEmail,
                      const std::string &ip)
{
    auto urlEncode = [](const std::string &in) -> std::string
    {
        std::ostringstream oss;
        for (unsigned char c : in)
        {
            // encode space and all non-unreserved characters per RFC 3986.
            if ((c >= 'A' && c <= 'Z') ||
                (c >= 'a' && c <= 'z') ||
                (c >= '0' && c <= '9') ||
                c == '-' || c == '_' || c == '.' || c == '~' || c == '/')
            {
                oss << c;
            }
            else
            {
                char buf[4];
                snprintf(buf, sizeof(buf), "%02X", c);
                oss << '%' << buf;
            }
        }
        return oss.str();
    };
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
        body += "<tr><td><a href=\"/\" class=\"home\">Home (/)</a></td><td></td><td></td></tr>";                 // root
        body += "<tr><td><a href=\"../\" class=\"upfolder\">Parent Directory (../)</a></td><td></td><td></td></tr>"; // parent dir
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
    body += urlEncode(e);
        if (isDir)
        {
            body += "/"; // ensure link ends with slash for directories
            body += "\" class=\"directory\">" + display + "</a></td>";
        }
        else
        {
            body += "\" class=\"" + getFileTypeClass(e) + "\">" + display + "</a></td>";
        }

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
        } else
        {
            body += "<td></td><td></td>"; // empty size and date for directories
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