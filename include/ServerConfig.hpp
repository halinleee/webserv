#ifndef SERVERCONFIG_HPP
#define SERVERCONFIG_HPP

#include "LocationConfig.hpp" // LocationConfig 타입 사용(locations 맵 value, matchLocation)

#include <iosfwd>  // std::ifstream (parseServerDirective/endSequenceValid/생성자 선언)
#include <cstddef> // size_t (clientMaxBodySize, parseBody 매개변수)
#include <map>	   // std::map (errorPages, locations)
#include <string>  // std::string (statusMessage, prefixes 요소, 매개변수/리턴)
#include <vector>  // std::vector (prefixes, 여러 함수 매개변수)

/**
 * @class ServerConfig
 * @brief 설정 파일 내 하나의 server 블록을 표현한 구조체
 */
class ServerConfig
{
	private:
		size_t keepAliveTimeout;
		size_t clientMaxBodySize;
		std::map<unsigned int, std::string> errorPages;
		std::map<std::string, LocationConfig> locations;
		std::string statusMessage;
		std::vector<std::string> prefixes;

	private:
		void ServerConfig::setPrefixes(void);
		bool ServerConfig::matching(const std::string &url);
		bool parseKeepAlive(std::vector<std::string> &token, size_t max);
		/**
		 * @brief `error_page` 지시어를 검증하고 errorPages에 저장한다.
		 * @details token 형식은 다음과 같아야 한다.
		 * - error_page <status_code> <uri>
		 * 예: {"error_page", "404", "/errors/404.html"}
		 * @param token 공백 기준으로 분리된 지시어 토큰
		 * @return 유효하면 true, 아니면 false
		 */
		bool parseErrorPage(std::vector<std::string> &token);
		/**
		 * @brief `error_page` 지시어를 검증하고 errorPages에 저장한다.
		 * @details token 형식은 다음과 같아야 한다.
		 * - error_page <status_code> <uri>
		 * 예: {"error_page", "404", "/errors/404.html"}
		 * @param token 공백 기준으로 분리된 지시어 토큰
		 * @return 유효하면 true, 아니면 false
		 */
		bool parseBody(const std::vector<std::string> &token, size_t max);
		/**
		 * @brief server 블록 내부(indent 1) 지시어를 파싱한다.
		 * @details
		 * 허용 지시어:
		 * - client_max_body_size
		 * - error_page
		 * - location (location 블록 파싱은 LocationConfig에 위임)
		 * @param token 공백 기준으로 분리된 지시어 토큰
		 * @param configFile 열린 config 파일 스트림 (location 파싱 시 추가로 읽는다)
		 * @return 유효한 지시어이고 처리에 성공하면 true, 아니면 false
		 */
		bool parseServerDirective(std::vector<std::string> &token, std::ifstream &configFile);
		/**
		 * @brief `end` 이후에 올 수 있는 다음 블록을 검증하고 파싱 상태를 설정한다.
		 * @details
		 * `end` 다음에는 (빈 줄을 제외하고) 다음 server 헤더(`server <port>`)가 오거나,
		 * 파일이 종료되어야 한다. 그 외의 문자열이 오면 config 형식 오류로 처리한다.
		 *
		 * 이 함수는 configFile의 위치를 필요 시 다음 server 헤더 시작 지점으로 되돌린다(seekg).
		 *
		 * @param configFile 열린 config 파일 스트림
		 * @note 결과는 statusMessage에 다음 중 하나로 저장된다:
		 * - "server end" : 다음 server 블록이 이어짐
		 * - "file end"   : 파일 종료
		 * - "Config error: Invalid server block format" : 형식 오류
		 */
		void endSequenceValid(std::ifstream &configFile);
		void setPrefixes(void);

	public:
		ServerConfig()
		{
			clientMaxBodySize = 1000000;
			keepAliveTimeout = 75;
		};
		/**
		 * @brief server 블록 본문을 파싱하여 ServerConfig를 구성한다.
		 * @details
		 * 이 생성자는 server 헤더(server <port>) 다음 라인부터 읽기 시작하여,
		 * 들여쓰기 규칙에 따라 server 블록을 구성하는 지시어들을 처리한다.
		 *
		 * 처리 규칙(현재 구현 기준):
		 * - indent 1: server 지시어(client_max_body_size, error_page, location)
		 * - indent 0: "end"를 만나면 server 블록 종료
		 * - indent > 1 또는 indent == -1: 들여쓰기 오류로 실패
		 *
		 * @param configFile 열린 config 파일 스트림 (현재 server 블록의 본문을 읽는다)
		 * @param configLine 호출 시점의 라인 버퍼(입력/출력 용도)
		 * @note 파싱 결과/오류는 statusMessage에 저장되며, 예외는 던지지 않는다.
		 */
		ServerConfig(std::ifstream &configFile, std::string configLine);
		const size_t &getClientMaxBodySize() const { return clientMaxBodySize; }
		const std::map<unsigned int, std::string> &getErrorPages() const { return errorPages; }
		const std::map<std::string, LocationConfig> &getLocations() const { return locations; }
		const std::string &getStatusMessage() const { return statusMessage; }

	public:
		bool matching(const std::string &url);
		LocationConfig matchLocation;
};

#endif