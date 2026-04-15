# ifndef CONFIG_HPP
# define CONFIG_HPP

#include <string>
#include <map>
#include <vector>
#include <set>
#include <cstddef>
#include <fstream> //ifstream
#include <iostream>
#include <sstream>
#include <cctype>


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
 * @class LocationConfig
 * @brief 설정 파일 내 하나의 server 블록을 표현한 구조체
 * 
 * root: 요청한 리소스를 찾을 기준 디렉토리 경로 (ex: root ./www/site1;)
 * 
 * index: 디렉토리 요청 시 기본으로 반환할 인덱스 파일명 (ex: index index.html;)
 * 
 * autoindex: 디렉토리 요청 시 파일 목록을 자동 생성하여 보여줄지에 대한 여부
		 * true이면 디렉토리 목록을 반환하고,
 		 * false이면 인덱스 파일이 없을 때 보통 에러로 처리한다.
 * 	
 * methods: 이 location에서 허용되는 HTTP 메서드 목록
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
 * 
 * upload_dir: 클라이언트가 업로드한 파일을 저장할 디렉토리 경로
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
 * 
 * redirect: 이 location에 대한 리다이렉트 설정 정보
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
class LocationConfig
{
	private:
		std::string root;
		std::string index;
		bool autoIndex;
		std::set<HttpMethod> methods;
		std::string uploadDir;
		int redirectStatusCode;
		std::string redirectPath;
		std::string cgiExtension;
		std::string cgiPath;


	public:
		LocationConfig()
		{
			root = 
			autoIndex = false;
			methods.insert(METHOD_GET); //메서드 추가할때 clear로 꼭 초기화
    		methods.insert(METHOD_POST);
    		methods.insert(METHOD_DELETE);
			redirectStatusCode = 301;
		}
		void setRoot(std::string value) { root = value; }
		void setIndex(std::string value) { index = value; }
		void clearMethods() { methods.clear(); }
		void setMethod(HttpMethod method) { methods.insert(method); }
		void setAutoIndex(bool value) { autoIndex = value; }
		void setUploadDir(std::string value) { uploadDir = value; }
		void setRedirect(int code, std::string path) { redirectStatusCode= code, redirectPath = path; }
		void setCgi(std::string ext, std::string path) { cgiExtension = ext, cgiPath = path; }

		const std::string& getRoot() const { return root; }
        const std::string& getIndex() const { return index; }
        bool getAutoIndex() const { return autoIndex; }
        const std::set<HttpMethod>& getMethods() const { return methods; }
        const std::string& getUploadDir() const { return uploadDir; }
        int getRedirectStatusCode() const { return redirectStatusCode; }
        const std::string& getRedirectPath() const { return redirectPath; }
        const std::string& getCgiExtension() const { return cgiExtension; }
        const std::string& getCgiPath() const { return cgiPath; }
};

/**
 * @class ServerConfig
 * @brief 설정 파일 내 하나의 server 블록을 표현한 구조체
 */
class ServerConfig
{
	public:
		size_t clientMaxBodySize;
		std::map<int, std::string> errorPages;
		std::map<std::string, LocationConfig> locations;

	public:
		 ServerConfig()
    	{
			clientMaxBodySize = 10000000;
			errorPages = std::map<int, std::string>();
			locations = std::map<std::string, LocationConfig>();
    	}
		void setClientMaxBodySize(size_t num) { clientMaxBodySize = num; }
		void setErrorPages(int key, std::string value) { errorPages[key] = value; }
};

/** 
 * @class Config
 * @brief 여러 서버 설정을 관리하는 최상위 구조체
 */
class Config
{
	public:
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
		
		int validateConfig(std::ifstream& configFile);
		int isEndSequenceValid(std::ifstream &configFile);
		bool serverDirectiveValidate(unsigned int key, const std::vector<std::string>& token, std::ifstream& configFile);
		bool listenBodyValidate(unsigned int key, const std::vector<std::string>& token, size_t max);
		bool errorPageValidate(unsigned int key, const std::vector<std::string>& token);
		bool locationValidate(unsigned int key, const std::vector<std::string>& locationToken, std::ifstream& configFile);
};


bool isBlankLine(const std::string &line);
int countIndent(const std::string& line);
bool removeChar (std::string &value, char delim);
std::vector<std::string> ftSplit(const std::string& line, char delim);
bool isNumber(const std::string &s);
bool toInt (const std::string &s, unsigned int &value);

# endif