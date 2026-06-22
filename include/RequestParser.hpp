#ifndef REQUESTPARSER_HPP
#define REQUESTPARSER_HPP

#include "type.hpp"
#include "Request.hpp"

/**
 * HTTP request 파싱할 때 사용하는 길이 제한 상수
 */
#define MAX_STARTLINE_LENGTH 8192
#define MAX_URI_LENGTH 4096
#define MAX_HEADER_LINE_LENGTH 1024
#define MAX_HEADER_SECTION_LENGTH (16 * 1024)
#define MAX_CLIENT_BODY_LENGTH 1000000			// 가을이가 파싱한 value 이용(TODO)

/**
 * @brief HTTP 요청 파싱 상태를 나타내는 열거형
 * 
 * REQ_STARTLINE: 요청 라인 파싱 단계
 * REQ_HEADERS: 헤더 파싱 단계
 * REQ_BODY: 바디 파싱 단계
 * REQ_DONE: 파싱 완료
 * REQ_ERROR: 파싱 중 오류 발생
 */
enum ParseState // TODO: class 내에 삽입 고려
{
	REQ_STARTLINE,
	REQ_HEADERS,
	REQ_BODY,
	REQ_DONE,
	REQ_ERROR
};

/**
 * @brief start-line 파싱 시 각 요소를 담을 임시 구조체
 */
typedef struct
{
	std::string method;
	std::string target;
	std::string version;
} ReqLine;

class RequestParser
{
	private:
		ParseState parseState;
		Status statusCode;

		ReqLine tmpReqLine;
		std::map<std::string, strVec> tmpHeaders;

		Request parsedReq;
		size_t chunkRemaining;
		bool inTrailer;

		// request line
		/**
		 * @brief HTTP request의 start-line을 파싱
		 * 
		 * 버퍼에서 leading CR/LF를 제거한 후, CRLF까지 한 줄을 추출한 후
		 * method, URI, version으로 나눈 후 각각 파싱한다.
		 *
		 * @param buf start-line을 추출하여 파싱할 버퍼
		 * @return false: 데이터가 아직 다 안 들어와서 파싱 불가
		 * 		   true: 파싱 완료 (정상 또는 에러)
		 * 
		 * @details CRLF가 아직 안 들어온 경우 false 리턴
		 * 			start-line 분리에 실패하거나 method, URI, version 검증에
		 * 			실패하면 state를 REQ_ERROR로 상태 설정
		 * 			파싱에 성공하면 REQ_HEADERS로 상태 설정
		 */
		bool parseStartline(CharDq& buf);

		/**
		 * @brief start-line을 method, URI, version으로 분리
		 * 
		 * @param line 분리할 start-line
		 * @param req 결과를 저장할 ReqLine 구조체
		 * 
		 * @return 형식이 올바르면 true, 아니면 false
		 * 
		 * @details
		 * 		CR이 포함되거나 길이가 MAX_STARTLINE_LENGTH를 초과하면 false
		 * 		공백(SP)은 정확히 두 개여야 하며, 3개 이상의 토큰이 존재하면 false
		 * 		성공 시 method, target, version이 req에 저장됨
		 */
		bool splitStartline(const std::string& line, ReqLine& req);

		/**
		 * @brief method 파싱 및 검증
		 * 
		 * 메소드를 파싱한 후 parsedReq에 저장. 적절한 statusCode도 설정
		 * 
		 * @param method 파싱할 메소드 문자열
		 * 
		 * @return 우리 서버가 지원하는 메소드이면 true, 아니면 false
		 * 
		 * @details
		 * 		GET, POST, DELETE는 서버가 지원하는 메소드
		 * 		나머지 메소드들은 statusCode를 501로 설정한 후 리턴
		 * 		그 외의 알 수 없는 메소드들은 statusCode를 400으로 리턴한 후 리턴
		 */
		bool parseMethod(const std::string& method);

		/**
		 * @brief URI 파싱 및 검증
		 * 
		 * @param target 파싱할 URI 문자열
		 * 
		 * @return 유요한 URI면 true, 아니면 false
		 * 
		 * @details
		 * 		빈 문자열이거나 '/'로 시작하지 않으면 false
		 * 		'?'는 최대 1개만
		 * 		길이가 MAX_URI_LENGTH를 초과하면 statusCode를 STATUS_URI_LONG로 설정하고 false
		 * 		ASCII 범위(32~126)를 벗어나면 false
		 * 		퍼센트 인코딩이 유효하지 않으면 false
		 * 		path/query로 분리 후, path에 "//" 또는 "." ".." 가 있으면 false
		 * 		path는 percent-decoding 후 parsedReq.path에 저장
		 * 		query가 존재하면 parsedReq.query에 저장
		 */
		bool parseURI(const std::string& target);

		/**
		 * @brief 퍼센트 인코딩이 유효한 형태로 되어 있는지 검증
		 * 
		 * 		  '%' 뒤에 16진수 두 자리수가 따라나와야 유효함
		 * 
		 * @param target 검증할 URI
		 * 
		 * @return 유효한 인코딩이면 true, 아니면 false
		 */
		bool isValidPercentEncoding(const std::string& target);

		bool splitURI(const std::string& target, std::string& path, std::string& query);

		/**
		 * @brief 퍼센트 인코딩을 디코딩
		 * 
		 * 		  디코딩 결과에 널문자가 있거나 연속된 슬래시가 있다면 false
		 * 
		 * @param path 디코딩할 경로
		 * @param result 디코딩된 경로를 저장할 문자열
		 * 
		 * @return 디코딩된 결과가 유효하면 true, 아니면 false
		 */
		bool percentDecode(const std::string& path, std::string& result);

		/**
		 * @brief version 형식 검증 및 버전이 1.1인지 확인
		 * 
		 * @param version 버전 검증할 문자열
		 * 
		 * @return HTTP/1.1이면 true, 형식이 유효하지 않거나 버전이 1.1이 아니면 false
		 */
		bool parseVersion(const std::string& version);

		// headers
		bool parseHeaders(CharDq& buf);
		bool parseKeyValue(const std::string& line, std::string& key, std::string& value);
		bool validateHeaders();
		bool parseHost(const std::string& raw);
		///////////// max client body length는 config에서 파싱된 값을 사용해야함. 수정 필요
		bool validateContentLength(const strVec& cl);
		bool validateTransferEncoding(const strVec& te);
		void transferHeaders();

		// body
		bool parseBody(CharDq& buf);
		bool parsePlainBody(CharDq& buf);
		bool parseChunkedBody(CharDq& buf);

		void handleError();

	public:
		RequestParser();
		~RequestParser();

		void parse(CharDq& buf);
		void clear();

		ReqParseResult getState() const;
		Request getRequest() const;
};

#endif