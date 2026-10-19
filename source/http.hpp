#include <iostream>
#include <string>
#include <fstream>
#include "Log.hpp"
#include <regex>
#include <vector>
#include <unordered_map>
#include <sys/stat.h>
#include <sys/types.h>
using namespace LogMoudle;
std::unordered_map<int, std::string> _statu_msg =
    {
        {100, "Continue"},
        {101, "Switching Protocol"},
        {102, "Processing"},
        {103, "Early Hints"},
        {200, "OK"},
        {201, "Created"},
        {202, "Accepted"},
        {203, "Non-Authoritative Information"},
        {204, "No Content"},
        {205, "Reset Content"},
        {206, "Partial Content"},
        {207, "Multi-Status"},
        {208, "Already Reported"},
        {226, "IM Used"},
        {300, "Multiple Choice"},
        {301, "Moved Permanently"},
        {302, "Found"},
        {303, "See Other"},
        {304, "Not Modified"},
        {305, "Use Proxy"},
        {306, "unused"},
        {307, "Temporary Redirect"},
        {308, "Permanent Redirect"},
        {400, "Bad Request"},
        {401, "Unauthorized"},
        {402, "Payment Required"},
        {403, "Forbidden"},
        {404, "Not Found"},
        {405, "Method Not Allowed"},
        {406, "Not Acceptable"},
        {407, "Proxy Authentication Required"},
        {408, "Request Timeout"},
        {409, "Conflict"},
        {410, "Gone"},
        {411, "Length Required"},
        {412, "Precondition Failed"},
        {413, "Payload Too Large"},
        {414, "URI Too Long"},
        {415, "Unsupported Media Type"},
        {416, "Range Not Satisfiable"},
        {417, "Expectation Failed"},
        {418, "I'm a teapot"},
        {421, "Misdirected Request"},
        {422, "Unprocessable Entity"},
        {423, "Locked"},
        {424, "Failed Dependency"},
        {425, "Too Early"},
        {426, "Upgrade Required"},
        {428, "Precondition Required"},
        {429, "Too Many Requests"},
        {431, "Request Header Fields Too Large"},
        {451, "Unavailable For Legal Reasons"},
        {501, "Not Implemented"},
        {502, "Bad Gateway"},
        {503, "Service Unavailable"},
        {504, "Gateway Timeout"},
        {505, "HTTP Version Not Supported"},
        {506, "Variant Also Negotiates"},
        {507, "Insufficient Storage"},
        {508, "Loop Detected"},
        {510, "Not Extended"},
        {511, "Network Authentication Required"}};
std::unordered_map<std::string, std::string> _mime_msg =
    {
        {".aac", "audio/aac"},
        {".abw", "application/x-abiword"},
        {".arc", "application/x-freearc"},
        {".avi", "video/x-msvideo"},
        {".azw", "application/vnd.amazon.ebook"},
        {".bin", "application/octet-stream"},
        {".bmp", "image/bmp"},
        {".bz", "application/x-bzip"},
        {".bz2", "application/x-bzip2"},
        {".csh", "application/x-csh"},
        {".css", "text/css"},
        {".csv", "text/csv"},
        {".doc", "application/msword"},
        {".docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
        {".eot", "application/vnd.ms-fontobject"},
        {".epub", "application/epub+zip"},
        {".gif", "image/gif"},
        {".htm", "text/html"},
        {".html", "text/html"},
        {".ico", "image/vnd.microsoft.icon"},
        {".ics", "text/calendar"},
        {".jar", "application/java-archive"},
        {".jpeg", "image/jpeg"},
        {".jpg", "image/jpeg"},
        {".js", "text/javascript"},
        {".json", "application/json"},
        {".jsonld", "application/ld+json"},
        {".mid", "audio/midi"},
        {".midi", "audio/x-midi"},
        {".mjs", "text/javascript"},
        {".mp3", "audio/mpeg"},
        {".mpeg", "video/mpeg"},
        {".mpkg", "application/vnd.apple.installer+xml"},
        {".odp", "application/vnd.oasis.opendocument.presentation"},
        {".ods", "application/vnd.oasis.opendocument.spreadsheet"},
        {".odt", "application/vnd.oasis.opendocument.text"},
        {".oga", "audio/ogg"},
        {".ogv", "video/ogg"},
        {".ogx", "application/ogg"},
        {".otf", "font/otf"},
        {".png", "image/png"},
        {".pdf", "application/pdf"},
        {".ppt", "application/vnd.ms-powerpoint"},
        {".pptx", "application/vnd.openxmlformats-officedocument.presentationml.presentation"},
        {".rar", "application/x-rar-compressed"},
        {".rtf", "application/rtf"},
        {".sh", "application/x-sh"},
        {".svg", "image/svg+xml"},
        {".swf", "application/x-shockwave-flash"},
        {".tar", "application/x-tar"},
        {".tif", "image/tiff"},
        {".tiff", "image/tiff"},
        {".ttf", "font/ttf"},
        {".txt", "text/plain"},
        {".vsd", "application/vnd.visio"},
        {".wav", "audio/wav"},
        {".weba", "audio/webm"},
        {".webm", "video/webm"},
        {".webp", "image/webp"},
        {".woff", "font/woff"},
        {".woff2", "font/woff2"},
        {".xhtml", "application/xhtml+xml"},
        {".xls", "application/vnd.ms-excel"},
        {".xlsx", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
        {".xml", "application/xml"},
        {".xul", "application/vnd.mozilla.xul+xml"},
        {".zip", "application/zip"},
        {".3gp", "video/3gpp"},
        {".3g2", "video/3gpp2"},
        {".7z", "application/x-7z-compressed"}};
class Util
{
public:
    static ssize_t Split(const std::string &str, const std::string &sep, std::vector<std::string> *arry)
    {
        ssize_t offset = 0;
        while (offset < str.size())
        {
            ssize_t pos = str.find(sep, offset);
            if (pos == std::string::npos)
            {
                if (pos == str.size())
                {
                    break;
                }
                arry->push_back(str.substr(offset));
                return arry->size();
            }
            if (pos == offset)
            {
                offset = pos + sep.size();
                continue;
            }
            arry->push_back(str.substr(offset, pos - offset));
            offset = pos + sep.size();
            continue;
        }
        return arry->size();
    }
    static bool ReadFile(const std::string &filename, std::string *buf)
    {
        std::ifstream ifs(filename, std::ios::binary);
        if (ifs.is_open() == false)
        {
            LOG(LogLevel::ERROR) << "open file error";
            return false;
        }
        ssize_t size = 0;
        ifs.seekg(0, ifs.end);
        size = ifs.tellg();
        buf->resize(size);
        ifs.seekg(0, ifs.beg);
        ifs.read(&(*buf)[0], size);
        if (ifs.good() == false)
        {
            LOG(LogLevel::ERROR) << "read file error";
            ifs.close();
            return false;
        }
        ifs.close();
        return true;
    }
    static bool WriteFile(const std::string &filename, const std::string &buf)
    {
        std::ofstream ofs(filename, std::ios::binary | std::ios::trunc);
        if (ofs.is_open() == false)
        {
            LOG(LogLevel::ERROR) << "open file error";
            return false;
        }
        ofs.write(buf.c_str(), buf.size());
        if (ofs.good() == false)
        {
            LOG(LogLevel::ERROR) << "write file error";
            ofs.close();
            return false;
        }
        ofs.close();
        return true;
    }
    static std::string UrlEncode(const std::string &url, bool convert_space_to_plus)
    {
        std::string ret;
        for (auto &v : url)
        {
            if (v == '.' || v == '_' || v == '-' || v == '~' || isalnum(v))
            {
                ret += v;
                continue;
            }
            if (v == ' ' && convert_space_to_plus == true)
            {
                ret += '+';
                continue;
            }
            char c[4] = {0};
            snprintf(c, 4, "%%%02x", v);
            ret += c;
        }
        return ret;
    }
    static char HEXTOI(char c)
    {
        if (c >= '0' && c <= '9')
        {
            return c - '0';
        }
        if (c >= 'a' && c <= 'z')
        {
            return c - 'a' + 10;
        }
        if (c >= 'A' && c <= 'Z')
        {
            return c - 'A' + 10;
        }
    }
    static std::string UrlDecode(const std::string &url, bool convert_space_to_plus)
    {
        std::string ret;
        for (int i = 0; i < url.size(); i++)
        {
            if (url[i] == '+' && convert_space_to_plus == true)
            {
                ret += ' ';
                continue;
            }
            if (url[i] == '%' && (i + 2) < url.size())
            {
                char v1 = HEXTOI(url[i + 1]);
                char v2 = HEXTOI(url[i + 2]);
                char v = v1 * 16 + v2;
                ret += v;
                i += 2;
                continue;
            }
            ret += url[i];
        }
        return ret;
    }
    static std::string StatuDesc(int statu)
    {
        auto it = _statu_msg.find(statu);
        if (it != _statu_msg.end())
        {
            std::string ret = it->second;
            return ret;
        }
        return "Unknow";
    }
    static std::string ExtMime(const std::string &filename)
    {
        int pos = filename.find_last_of('.');
        if (pos == std::string::npos)
        {
            return "application/octet-stream";
        }
        std::string temp = filename.substr(pos);
        auto it = _mime_msg.find(temp);
        if (it == _mime_msg.end())
        {
            return "application/octet-stream";
        }
        return it->second;
    }
    static bool IsDirectory(const std::string &filename)
    {
        struct stat st;
        int ret = stat(filename.c_str(), &st);
        if (ret < 0)
        {
            return false;
        }
        return S_ISDIR(st.st_mode);
    }
    static bool IsRegular(const std::string &filename)
    {
        struct stat st;
        int ret = stat(filename.c_str(), &st);
        if (ret < 0)
        {
            return false;
        }
        return S_ISREG(st.st_mode);
    }
    static bool ValidPath(const std::string &path)
    {
        int level = 0;
        std::vector<std::string> temp;
        Split(path, "/", &temp);
        for (auto &V : temp)
        {
            if (V == "..")
            {
                level--;
                if (level < 0)
                {
                    return false;
                }
                continue;
            }
            level++;
        }
        return true;
    }
};

class HttpResponse
{
public:
    int _statu;
    bool _redirect_flag;
    std::string _body;
    std::string _redirect_url;
    std::unordered_map<std::string, std::string> _headers;

public:
    HttpResponse()
        : _statu(200), _redirect_flag(false)
    {
    }
    HttpResponse(int statu)
        : _statu(statu), _redirect_flag(false)
    {
    }
    void Reset()
    {
        _statu = 200;
        _redirect_flag = false;
        _body.clear();
        _redirect_url.clear();
        _headers.clear();
    }
    void SetHeader(const std::string &key, const std::string &val)
    {
        _headers.insert({key, val});
    }
    bool HasHeader(const std::string &key)
    {
        auto it = _headers.find(key);
        if (it != _headers.end())
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    std::string GetHeader(const std::string &key)
    {
        auto it = _headers.find(key);
        if (it == _headers.end())
        {
            return "";
        }
        return it->second;
    }
    void SetContent(const std::string &body, const std::string &type = "text/html")
    {
        _body = body;
        SetHeader("Content-Type", type);
    }
    void SetRedirect(const std::string &url, int statu = 302)
    {
        _redirect_flag = true;
        _redirect_url = url;
        _statu = statu;
    }
    bool Close()
    {
        if (HasHeader("Connection") == true && GetHeader("Connection") == "keep-alive")
        {
            return false;
        }
        return true;
    }
};

class HTTPRequest
{
public:
    std::string _body;   // 请求正文
    std::string _method; // 请求方法
    std::string _path;   // 请求资源路径
    std::string _version;
    std::smatch _matches;
    std::unordered_map<std::string, std::string> _headers;
    std::unordered_map<std::string, std::string> _params;

public:
    void SetHeader(const std::string &key, const std::string &val)
    {
        _headers.insert({key, val});
    }
    bool HasHeader(const std::string &key)
    {
        auto it = _headers.find(key);
        if (it != _headers.end())
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    std::string GetHeader(const std::string &key)
    {
        auto it = _headers.find(key);
        if (it == _headers.end())
        {
            return "";
        }
        return it->second;
    }
    void SetContent(const std::string &body, const std::string &type = "text/html")
    {
        _body = body;
        SetHeader("Content-Type", type);
    }
    void SetParam(const std::string &key, const std::string &val)
    {
        _params.insert({key, val});
    }
    bool HasParam(const std::string &key)
    {
        auto it = _params.find(key);
        if (it != _params.end())
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    std::string GetParam(const std::string &key)
    {
        auto it = _params.find(key);
        if (it == _params.end())
        {
            return "";
        }
        return it->second;
    }
    size_t ContentLength()
    {
        bool ret = HasHeader("Content-Length");
        if (ret == false)
        {
            return 0;
        }
        std::string temp = GetHeader("Content-Length");
        return std::stol(temp);
    }
    bool Close()
    {
        if (HasHeader("Connection") == true && GetHeader("Connection") == "keep-alive")
        {
            return false;
        }
        return true;
    }
};

typedef enum
{
    RECV_HTTP_ERROR,
    RECV_HTTP_LINE,
    RECV_HTTP_HEAD,
    RECV_HTTP_BODY,
    RECV_HTTP_OVER
} HttpRecvStatu;

class HTTPContext
{
private:
    HttpRecvStatu _rcv_statu; // 解析状态
    int _resp_statu;          // 相应状态码
    HTTPRequest _request;

private:
    bool ParseHttpLine(const std::string &line)
    {
        std::regex res("(GET|HEAD|POST|PUT|DELETE) ([^?]*)(?:\\?(.*))? (HTTP\\/1\\.[01])(?:\n|\r\n)?", std::regex::icase);
        std::smatch matches;
        bool ret = std::regex_match(line, matches, res);
        if (ret == false)
        {
            return false;
        }
        // 0 : get /baidu/login?user=xiaoming&pass=123123 HTTP/1.1
        // 1 : get
        // 2 : /baidu/login
        // 3 : user=xiaoming&pass=123123
        // 4 : HTTP/1.1
        _request._method = matches[1];
        std::transform(_request._method.begin(), _request._method.end(), _request._method.begin(), ::toupper);
        _request._path = Util::UrlDecode(matches[2],false);
        _request._version = matches[4];
        std::vector<std::string> temp;
        Util::Split(matches[3],"&",&temp);
        for(auto& v: temp)
        {
            
        }
    }
};