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

/**
 * @brief 각 status에 대해서 키워드로 관리하기 위해서 enum을 설정 후 사용
 * @var RET_ERROR 에러가 발생했을때 status로 숫자로는 0을 가지고 있음
 * @var RET_OK 정상 동작했을때 status로 숫자로는 1을 가지고 있음
 * @var RET_RE 정상 동작은 했지만 pipe의 총길이 제한, send수 제한 등 한번에 다 보내지 못했을 경우 status로 숫자로는 2를 가지고 있음
 * 
 * @todo 나중에 각 status code에 대해서 확인한 후 status code의 숫자로 지정해 넘겨주도록 변경필요
 */
enum  RetStatus 
{
    RET_ERROR = 0,
    RET_OK = 1,
    RET_RE = 2
};

/**
 * @brief timeOut을 관리하기 위해서 활동시간과 timeOut시간을 가지고 있는 구조체
 */
struct timeValue
{
    time_t connetionTimeOut;
    time_t readTimeout;
    time_t writeTimeout;
    time_t keepAliveTimeout;
    time_t cgiTimeout;
};

enum Status
{
    STATUS_UNDEFINED = 0, //// 이렇게 해도 되는지 검증 필요
    STATUS_BAD_REQUEST = 400,
    STATUS_NOT_FOUND = 404,
    STATUS_URI_LONG = 414,
    STATUS_NOT_IMPLEMENTED = 501,
    STATUS_HTTP_VERSION = 505,
    STATUS_HEADER_TOO_LARGE = 431,
    STATUS_PAYLOAD_TOO_LARGE = 413
};

enum ReqParseResult
{
    REQ_PARSE_ERROR,
    REQ_PARSE_DONE,
    REQ_PARSE_INCOMPLETE
};

/**
 * @brief HTTP 요청의 메소드를 구분하는 열거형
 * 
 * @note 서버에서 구현하는 메소드는 GET, POST, DELETE
 * 
 *       나머지 메소드는 에러 응답을 보내기 위해 필요
 */
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

/**
 * @brief recv로 수신한 원본 HTTP 요청 데이터를 바이트 단위로 누적 저장하는 deque 컨테이너
 * 
 * 논블로킹 I/O를 사용하므로 데이터가 잘려서 도착할 수 있습니다.
 * 
 * Client 클래스 내부에서 버퍼 역할을 수행하며, 파서(Parser)에서 내용을 전달하기 위한 공간으로 사용하고 있습니다.
 */
typedef std::deque<unsigned char> CharDq;

/**
 * @brief 이벤트가 발생한 CGI 파이프 FD(key)를 통해 해당 요청을 소유한 클라이언트의 FD(value)를 역추적하기 위한 맵 타입
 * 
 * epoll 이벤트가 파이프 FD를 반환했을 때, 이 파이프가 속한 Client 객체를 빠르게 찾기 위해 Server 클래스에 멤버로 존재합니다.
 */
typedef std::map<int, int> IntMap;

/**
 * @brief FD를 인덱스로 동적 할당된 Client 객체 포인터를 가지고 있는 vector 컨테이너
 * 
 * 현재 서버에 접속되어 세션을 유지 너중인 모든 클라이언트들의 목록입니다. 특정 클라이언트 소켓에 이벤트 발생 시 해당 클라이언트의 컨텍스트(데이터, 상태)를 즉시 불러오기 위해 사용됩니다.
 */
typedef std::vector<Client *> ClientVec;

/**
 * @brief body의 내용을 char로 가지고 있는 Vector 컨테이너
 * 
 * body의 내용을 pipe에 write쓰기 위해 pipeWrite함수로 가야하는데 들고 있는 공간 
 */
typedef std::vector<char> bodyVec;

/**
 * @brief FD의 내용을 담고 있는 Vector 컨테이너
 * 
 * pipe의 FD를 가지고 자식 프로세스에서 FD를 close하기 위해서 가지고 있음(여러 개의 클라이언트가 동시에 들어와서 Fork할때 다른 pipe도 가지고 들어갈 경우)
 */
typedef std::vector<FD> FdVec;

/**
 * @brief 환경변수의 KEY(string)와 VALUE(string) 쌍을 관리하는 맵 타입
 * 
 * Server 클래스는 부모 프로세스의 환경변수를 저장하고, Client는 여기에 HTTP 요청에서 추출한 CGI Meta-Variables를 병합해 보관합니다.
 * 
 * 예: key = "PATH", value = "/usr/bin"
 * 
 * 최종적으로 execve 호출 시 char** 배열 형태로 변환되어 사용됩니다.
 */
typedef std::map<std::string, std::string> EnvMap;

/**
 * @brief string 객체를 원소로 갖는 vector
 *
 * request header의 값들을 임시로 저장하기 위해 사용
 */
typedef std::vector<std::string> strVec;

/**
 * @brief string 객체를 원소로 갖는 vector
 *
 * request header의 값들을 임시로 저장하기 위해 사용
 */
typedef std::vector<std::string> strVec;

#endif