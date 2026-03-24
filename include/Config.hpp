# ifndef CONFIG_HPP
# define CONFIG_HPP

#include <string>
#include <map>
#include <vector>
#include <set>
#include <cstddef>

struct RedirectConfig
{
	/**
	 * @var status_code
	 * @brief 리다이렉트 응답에 사용할 HTTP 상태 코드
	 * 
	 * 주로 301(영구이동), 302(Found) 등의 값이 사용된다.
	 * 
	 * ex: return 301 /files;
	 * 
	 * 예시 설정에서는 status_code에 301이 저장됨.
	 */
	int statusCode;
	/**
	 * @var target
	 * @brief 클라이언트가 이동해야 할 대상 경로(URL)
	 * 
	 * 리다이렉트 응답의 Location 헤더 값으로 사용됨
	 * 
	 * ex: return 301 /files;
	 * 
	 * 예시 설정에서는 target에 "/files"가 저장되며,
	 * 서버는 다음과 같은 응답을 생성할 수 있다.
	 * 
	 * HTTP/1.1 301 Moved Permanently
	 * Location: /files
	 */
	std::string target;
};

enum HttpMethod
{
	/**
	 * @brief GET 요청
	 * 
	 * 서버에 리소스를 요청할 때 사용되며,
	 * 주로 정적 파일 조회, 페이지 조회, 디렉토리 탐색 등에 사용된다.
	 */
	METHOD_GET,
	/**
	 * @brief POST 요청
	 * 
	 * 서버에 데이터를 전송할 때 사용되며,
	 * 주로 파일 업로드, 폼 전송, CGI 입력 데이터 전달 등에 사용된다.
	 */
	METHOD_POST,
	/**
	 * @brief DELETE 요청
	 * 
	 * 서버의 특정 리소스를 삭제할 때 사용된다.
	 * 업로드된 파일을 제거할 때 사용된다.
	 */
	METHOD_DELETE
};

/**
 * @struct LocationConfig
 * @brief 설정 파일 내 하나의 server 블록을 표현한 구조체
 */
struct LocationConfig
{
	/**
	 * @var prefix
	 * @brief 이 location이 매칭할 URI 경로 접두사
	 * 
	 * ex: "/", "/files", "/upload", "/redir" 등
	 */
	std::string prefix;

	/**
	 * @var root
	 * @brief 요청한 리소스를 찾을 기준 디렉토리 경로
	 * 
	 * ex: root ./www/site1;
	 */
	std::string root;
	/**
	 * @var has_root
	 * @brief root 값이 설정되었는지 여부
	 */
	bool hasRoot;
	
	/**
	 * @var index
	 * @brief 디렉토리 요청 시 기본으로 반환할 인덱스 파일명
	 * 
	 * ex: index index.html;
	 */
	std::string index;
	/**
	 * @var has_index
	 * @brief index 값이 설정되었는지 여부
	 */
	bool hasIndex;

	/**
	 * @var autoindex
	 * @brief 디렉토리 요청 시 파일 목록을 자동 생성하여 보여줄지에 대한 여부
	 *
	 * true이면 디렉토리 목록을 반환하고,
	 * false이면 인덱스 파일이 없을 때 보통 에러로 처리한다.
	 */
	bool autoIndex;
	/**
	 * @var has_autoindex
	 * @brief autoindex가 존재하는지에 대한 여부
	 */
	bool hasAutoIndex;

	/**
	 * @var methods
	 * @brief 이 location에서 허용되는 HTTP 메서드 목록
	 *
	 * 클라이언트 요청의 HTTP 메서드(GET, POST, DELETE)가
	 * 이 목록에 포함되어 있는 경우에만 요청을 처리함.
	 * 
	 * 설정 ex:
	 * 	methods GET POST;
	 * 
	 * 예시와 같이 설정된 경우:
	 * GET -> 허용
	 * POST -> 허용
	 * DELETE -> 거부
	 * 
	 * 허용되지 않은 메서드로 요청이 들어오면
	 * 보통 405 Method Not Allowed 응답을 반환함.
	 */
	std::set<HttpMethod> methods;
	/**
	 * @var has_methods
	 * @brief 허용 메서드가 설정되었는지 여부
	 */
	bool hasMethods;

	/**
	 * @var upload_dir
	 * @brief 클라이언트가 업로드한 파일을 저장할 디렉토리 경로
	 *
	 * 이 값은 주로 파일 업로드를 처리하는 location에서 사용되며,
	 * POST 요청의 본문(body)에 포함된 업로드 데이터를
	 * 서버의 어느 디렉토리에 저장할지 결정한다.
	 * 
	 * ex: upload_dir ./uploads;
	 * 
	 * 예시와 같이 설정된 경우, 해당 location으로 업로드된 파일은
	 * 서버의 "./uploads" 디렉토리에 저장된다.
	 * 
	 * 주의사항:
	 * 이 값은 업로드 기능이 필요한 location에서만 의미가 있으며,
	 * 일반적인 정적 파일 제공용 location에서는 사용되지 않을 수 있음
	 */
	std::string uploadDir;
	/**
	 * @var has_upload_dir
	 * @brief upload_dir 값이 설정되었는지 여부
	 * 
	 * 업로드 저장 경로가 설정되었는지에 대한 여부
	 */
	bool hasUploadDir;

	/**
	 * @var redirect
	 * @brief 이 location에 대한 리다이렉트 설정 정보
	 *
	 * 이 설정이 존재하면, 요청을 직접 처리하지 않고
	 * 클라이언트에게 다른 URL(/files)로 이동하라는 응답을 반환한다.
	 * 
	 * 예:
	 *   return 301 /files;
	 * 
	 * -> 요청: GET /redir
	 * -> 응답: 301이 영구적으로 이동되었습니다.
	 * 		location: /files
	 * 
	 * 이후 클라이언트는 /files로 다시 요청을 보낸다.
	 */
	RedirectConfig redirect;
	/**
	 * @var has_redirect
	 * @brief 리다이렉트 설정이 존재하는지 여부
	 * 
	 * true인 경우, 해당 location은 다른 설정(root, index 등)을
	 * 무시하고 리다이렉트 응답을 우선적으로 반환한다.
	 */
	bool hasRedirect;
};

/**
 * @struct ServerConfig
 * @brief 설정 파일 내 하나의 server 블록을 표현한 구조체
 */
struct ServerConfig
{
	/**
	 * @var client_max_body_size
	 * @brief 클라이언트가 서버로 전송할 수 있는 요청 바디의 최대 크기 (바이트 단위)
	 */
	size_t clientMaxBodySize;
	/**
	 * @var has_client_max_body_size
	 * @brief client_max_body_size 값이 설정되었는지 여부
	 */
	bool hasClientMaxBodySize;
	/**
	 * @var error_pages
	 * @brief HTTP 에러 코드에 대응하는 커스텀 에러 페이지 경로를 저장하는 맵
	 * 
	 * key : HTTP 상태 코드(ex: 404, 500)
	 * 
	 * value : 해당 에러 발생 시 클라이언트에게 반환할 파일 경로
	 * 
	 * (ex: 404 -> "/errors/404.html"
	 * 		500 -> "errors/500.html")
	 */
	std::map<int, std::string> errorPages;
	/**
	 * @var location
	 * @brief 해당 서버에 정의된 location 설정들의 목록
	 * 
	 * 각 원소는 하나의 location 블록에 해당하며,
	 * URI prefix와 그에 대한 처리 규칙을 저장한다.
	 */
	std::vector<LocationConfig> locations;
};

/** 
 * @struct Config
 * @brief 여러 서버 설정을 관리하는 최상위 구조체
 */
struct Config
{
	/**
	 * @var servers
	 * @brief 여러 서버 설정을 관리하는 최상위 맵
	 * 
	 * - key : 포트 번호(int)
	 * 
	 * - value : 해당 포트에서 동작하는 서버 설정(ServerConfig)
	 * 
	 * 각 ServerConfig는 다음과 같은 정보를 포함한다
	 * 
	 * - client_max_body_size
	 * 
	 * - error_page
	 * 
	 * - location 설정 등
	 * 
	 * - 기타 서버 관련 설정
	 */
	std::map<int, ServerConfig> servers;
	
};

# endif