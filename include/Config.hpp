#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "ServerConfig.hpp"

#include <netinet/in.h>
#include <iosfwd>
#include <cstddef>
#include <map>
#include <string>
#include <vector>

/**
 * @class Config
 * @brief 여러 서버 설정을 관리하는 최상위 구조체
 */
class Config
{
	private:
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
		std::map<in_port_t, ServerConfig> servers;
		std::string statusMessage;
		enum ParseStatus
		{
			PARSE_SERVER_END,
			PARSE_FILE_END,
			PARSE_ERROR
		};

	private:
		/**
		 * @brief `server <port>` 헤더의 포트 번호 형식을 검증한다.
		 * @param token 공백 기준으로 분리된 토큰. 예: {"server", "8080"}
		 * @param max 허용할 최대 포트 번호 (보통 65535)
		 * @return 포트가 유효하면 true, 아니면 false
		 */
		bool isValidListen(const std::vector<std::string>& token);
		/**
		 * @brief config 파일에서 server 블록 1개를 파싱하여 서버 맵에 등록한다.
		 * @details
		 * - server 헤더(`server <port>`)를 읽고 포트를 검증한다.
		 * - 이후 블록 본문은 ServerConfig 파서에 위임한다.
		 * @param configFile 열린 config 파일 스트림
		 * @retval PARSE_FILE_END: 더 이상 읽을 server가 없어서 전체 파싱이 끝난 경우
		 * @retval PARSE_SERVER_END: server 1개를 정상 처리했고 다음 server를 계속 파싱해야 하는 경우
		 * @retval PARSE_ERROR: 파싱 오류가 발생한 경우
		 * @note 오류 상세는 statusMessage에 저장된다.
		 */
		ParseStatus parseServerBlock(std::ifstream& configFile);

	public:
		/**
		 * @brief 기본 설정 파일(`./config/webserv.conf`)을 열고 전체 server 블록을 파싱한다.
		 * @details 파일 오픈 실패 또는 파싱 실패 시 statusMessage에 오류 상태를 저장하고 종료한다.
		 * @note 이 생성자는 예외를 던지지 않고 statusMessage로 상태를 전달한다.
		 */
		Config();
		const std::map<in_port_t, ServerConfig>& getConfig() const { return servers; }
		const std::string& getStatusMessage() const { return statusMessage; }
};

#endif