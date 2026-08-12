#ifndef TYPE_HPP
# define TYPE_HPP

#include <map>
#include <vector>
#include <deque>
#include <string>
#include <ctime>

class Socket;
class Client;

enum parseStatus
{
    PARSE_SERVER_END,
    PARSE_FILE_END,
    PARSE_ERROR
};

enum  RetStatus
{
    RET_ERROR = 0,
    RET_OK = 1,
    RET_RE = 2
};

struct timeValue
{
    time_t connectionTimeOut;
    time_t readTimeout;
    time_t writeTimeout;
    time_t keepAliveTimeout;
    time_t cgiTimeout;
};

enum Status
{
    STATUS_UNDEFINED = 0,

    STATUS_OK = 200,
    STATUS_CREATED = 201,
    STATUS_NO_CONTENT = 204,

    STATUS_MOVED_PERMANENTLY = 301,
    STATUS_FOUND = 302,
    STATUS_SEE_OTHER = 303,

    STATUS_BAD_REQUEST = 400,
    STATUS_FORBIDDEN = 403,
    STATUS_NOT_FOUND = 404,
    STATUS_METHOD_NOT_ALLOWED = 405,
    STATUS_REQUEST_TIMEOUT = 408,
    STATUS_PAYLOAD_TOO_LARGE = 413,
    STATUS_URI_LONG = 414,
    STATUS_HEADER_TOO_LARGE = 431,

    STATUS_INTERNAL_SERVER_ERROR = 500,
    STATUS_NOT_IMPLEMENTED = 501,
    STATUS_BAD_GATEWAY = 502,
    STATUS_SERVICE_UNAVAILABLE = 503,
    STATUS_GATEWAY_TIMEOUT = 504,
    STATUS_HTTP_VERSION = 505
};

enum ReqParseResult
{
    REQ_PARSE_ERROR,
    REQ_PARSE_DONE,
    REQ_PARSE_INCOMPLETE
};

enum FailMode
{
    FAIL_KEEP_ALIVE,
    FAIL_CLOSE
};

enum HttpMethod
{
    METHOD_GET,
    METHOD_POST,
    METHOD_DELETE,
    METHOD_PUT,
    METHOD_PATCH,
    METHOD_HEAD,
    METHOD_OPTIONS,
    METHOD_TRACE,
    METHOD_CONNECT,
    METHOD_INVALID
};

typedef int FD;

typedef std::deque<unsigned char> CharDq;

typedef std::map<int, int> IntMap;

typedef std::vector<Client *> ClientVec;

typedef std::vector<char> bodyVec;

typedef std::vector<FD> FdVec;

typedef std::map<std::string, std::string> EnvMap;

typedef std::vector<std::string> strVec;

#endif