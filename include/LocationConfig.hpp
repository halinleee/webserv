#ifndef LOCATIONCONFIG_HPP
#define LOCATIONCONFIG_HPP

#include <iosfwd>   // std::ifstream (LocationConfig(std::ifstream&))
#include <string>   // std::string (멤버/리턴 타입)
#include <vector>   // std::vector<std::string> (parseLocDir 매개변수)
#include <set>      // std::set<HttpMethod> (methods 멤버/리턴 타입)

/**
 * @brief 문자열로 된 HTTP 메서드를 HttpMethod enum으로 변환한다.
 *
 * @details 허용되는 메서드 문자열은 다음과 같다:
 * - "GET"    -> METHOD_GET
 * - "POST"   -> METHOD_POST
 * - "DELETE" -> METHOD_DELETE
 *
 * 그 외의 문자열은 변환에 실패한다.
 * 
 * 메서드의 기본값은 모두 허용임. 따라서 사용할 때 clear로 초기화하고 사용해야 함.
 *
 * @param s 변환할 메서드 문자열
 * @param out 변환 성공 시 결과가 저장될 HttpMethod 출력 인자
 * @return 변환 성공 시 true, 허용되지 않은 문자열이면 false
 */
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
		std::string redirectPath;
		std::string cgiExtension;
		std::string cgiPath;

		bool status;

	private:
		/**
		 * @brief location 블록 내부(indentation 2) 지시어를 파싱하고 멤버 값을 설정한다.
		 *
		 * @details 허용되는 지시어(현재 구현 기준):
		 * - root <path>
		 * - index <path>
		 * - methods <METHOD...>
		 * - autoindex on|off
		 * - upload_dir <path>
		 * - return <status_code> <path>
		 * - cgi_ext <ext> <path>
		 *
		 * `methods` 지시어는 등장 시 기존 methods를 clear한 뒤 토큰에 나온 메서드만 허용하도록 재설정한다.
		 *
		 * @param token 공백 기준으로 분리된 지시어 토큰(예: {"root", "./www"})
		 * @return 지시어/인자가 유효하고 값 설정에 성공하면 true, 아니면 false
		 */
		bool isValidMethodsForPrefix(const std::string& prefixToken, const std::vector<std::string> &methodsToken);
		bool parseHttpMethod(const std::string &s, HttpMethod &out);
		bool parseLocDir(std::vector<std::string> token, const std::string &prefixToken);


	public:
		LocationConfig()
		{
			autoIndex = false;
			methods.insert(METHOD_GET);
			methods.insert(METHOD_POST);
			methods.insert(METHOD_DELETE);
		}
		/**
		 * @brief location 블록 본문을 파싱하여 LocationConfig를 구성한다.
		 *
		 * @details
		 * 이 생성자는 `location <prefix>` 라인 다음부터 파일을 읽기 시작하여,
		 * 들여쓰기 규칙에 따라 location 지시어들을 처리한다.
		 *
		 * 들여쓰기 규칙:
		 * - indent 2: location 지시어(root, index, methods, autoindex, upload_dir, return, cgi_ext)
		 * - indent 0 또는 1: location 블록 종료로 간주하고, 파일 포인터를 해당 라인 시작으로 되돌린다(seekg)
		 * - indent == -1 또는 indent != 2: 들여쓰기 오류로 실패 처리
		 *
		 * 파싱 결과는 status 멤버에 기록된다.
		 * - status == true  : 정상 종료(블록 종료 조건 만족)
		 * - status == false : 파싱 중 오류 또는 EOF로 비정상 종료
		 *
		 * @param configFile 열린 config 파일 스트림 (location 블록 본문을 읽는다)
		 * @note 이 생성자는 예외를 던지지 않으며, 성공/실패는 status로 전달한다.
		 */
		LocationConfig(std::ifstream &configFile, const std::string &prefixToken);

		bool isOk() const { return status; }

		const std::string &getRoot() const { return root; }
		const std::string &getIndex() const { return index; }
		const bool &getAutoIndex() const { return autoIndex; }
		const std::set<HttpMethod> &getMethods() const { return methods; }
		const std::string &getUploadDir() const { return uploadDir; }
		const std::string &getRedirectPath() const { return redirectPath; }
		const std::string &getCgiExtension() const { return cgiExtension; }
		const std::string &getCgiPath() const { return cgiPath; }
};

#endif