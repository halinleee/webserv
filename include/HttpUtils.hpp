#ifndef HTTPUTILS_HPP
#define HTTPUTILS_HPP

#include "type.hpp"
#include <string>

namespace HttpUtils
{
	/**
	 * @brief 유효하지 않은 위치를 나타낼 때 사용하는 상수
	 * 
	 * size_t의 최댓값으로, 검색 실패의 경우 반환
	 */
	const size_t npos = static_cast<size_t>(-1);

	/**
	 * @brief 버퍼 앞 부분에 존재하는 CR/LF 제거
	 * 
	 * @param buf leading CR/LF가 제거될 버퍼
	 * 
	 * @note HTTP 요청 파싱 전 불필요한 CR/LF 제거 사용
	 */
	void consumeLeadingCRLF(CharDq& buf);

	/**
	 * @brief 버퍼에서 줄바꿈(CRLF)의 시작 인덱스를 찾아 반환
	 * 
	 * @param buf 줄바꿈을 찾을 CharDq 버퍼
	 * @return CRLF 시작 인덱스, 없으면 HttpUtils::npos
	 * 
	 * @note buf 크기가 2 미만이면 CRLF가 존재할 수 없음
	 */
	size_t findCRLF(const CharDq& buf);

	/**
	 * @brief 버퍼에서 줄바꿈줄바꿈(CRLFCRLF)의 시작 인덱스를 찾아 반환
	 * 
	 * @param buf 줄바꿈을 찾을 CharDq 버퍼
	 * @return CRLFCRLF 시작 인덱스, 없으면 HttpUtils::npos
	 * 
	 * @note buf 크기가 4 미만이면 CRLFCRLF가 존재할 수 없음
	 */
	size_t findCRLFCRLF(const CharDq& buf);

	/**
	 * @brief 버퍼에서 줄바꿈 전까지만 추출하여 반환
	 * 
	 * @param buf 줄바꿈 전까지의 line을 추출할 CharDq 버퍼
	 * @param end_pos 줄바꿈이 시작되는 인덱스
	 * @param end_size 줄바꿈의 크기 (2 또는 4)
	 * @return 추출한 문자열 (줄바꿈은 제외)
	 * 
	 * @warning 반드시 CRLF가 존재해야 함
	 * 			함수 호출 전에 CRLF가 있는지 확인하는 로직이 필요
	 */
	std::string extractLine(CharDq& buf, size_t end_pos, size_t end_size);

	/**
	 * @brief CR이 있는지 확인
	 * 
	 * @param line CR이 있는지 확인할 string
	 * @return CR이 있다면 true, 아니면 false
	 */
	bool hasCR(const std::string& line);

	/**
	 * @brief 어떤 문자가 16진수인지 확인
	 * 
	 * 주어진 문자가 0-9, A-F, a-f 범위에 속하는지 검사
	 * 
	 * @param c 16진수인지 확인할 문자
	 * @return 16진수이면 true, 아니면 false
	 */
	bool isHex(const char c);

	/**
	 * @brief 문자열에 연속되는 슬래시(/)가 있는지 검사
	 * 
	 * @param str 검사할 string
	 * @return 연속되는 슬래시가 있으면 true, 아니면 false
	 * 
	 * @note 빈 문자열이면 false
	 */
	bool hasConsecutiveSlashes(const std::string& str);

	/**
	 * @brief 경로에 .이나 ..이 있는지 검사
	 * 
	 * @param str 검사할 string
	 * @return dot segment가 존재하면 true, 아니면 false
	 */
	bool hasDotSegments(const std::string& str);

	/**
	 * @brief 16진수 문자를 10진수 숫자로 변환
	 * 
	 * @param c 숫자로 변환할 16진수 문자
	 * @return 변환된 정수 값(0~15), 유효하지 않으면 -1
	 */
	int hexToInt(const char c);

	/**
	 * @brief Tchar인지 판단
	 * 
	 * tchar = "!" / "#" / "$" / "%" / "&" / "'" / "*" / "+" / "-" / "." /
	 * 			"^" / "_" / "`" / "|" / "~" / DIGIT / ALPHA
	 * 
	 * @param c 판단할 문자
	 * @return Tchar면 true, 아니면 false
	 */
	bool isTchar(unsigned char c);

	/**
	 * @brief Vchar, SP, HTAB인지 판단
	 * 
	 * Vchar는 visible한 ascii char (0x21 ~ 0x7E)
	 * 
	 * @param c 판단할 문자
	 * @return Vchar/SP/HTAB이면 true, 아니면 false
	 */
	bool isVcharSpTab(unsigned char c);
}

#endif