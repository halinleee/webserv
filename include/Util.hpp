# ifndef UTIL_HPP
# define UTIL_HPP

#include <string>
#include <vector>

bool isValidFileName(const std::string &file);
/**
 * @brief location prefix(예: "/upload")를 검증하고 연속된 '/'를 1개로 정규화한다.
 *
 * @details
 * - 반드시 '/'로 시작해야 한다.
 * - 공백 문자가 포함되면 실패한다.
 * - 연속된 '/'는 하나로 축약한다.
 *
 * @param path 검증/정규화할 prefix 문자열
 * @return 유효하면 true, 아니면 false
 */
bool isValidNormalizePath(std::string& path);
/**
 * @brief 문자열 앞부분의 특정 문자(delim) 반복(들여쓰기)을 제거한다.
 *
 * @details value의 시작 부분에서 delim이 연속으로 등장하는 구간을 삭제한다.
 * 예) value="\t\troot", delim='\t'  -> value="root"
 *
 * @param value 대상 문자열
 * @param delim 제거할 문자(예: '\t')
 * @return 항상 true (현재 구현은 실패 케이스 없음)
 */
void removeIndent (std::string& value, char delim);
/**
 * @brief 문자열의 선행 탭('\t') 개수를 센다.
 *
 * @details
 * - 선행 탭은 들여쓰기로 계산한다.
 * - 선행 공백(' ')이 발견되면 들여쓰기 규칙 위반으로 -1을 반환한다.
 * - 탭/공백이 아닌 문자를 만나면 카운트를 종료한다.
 *
 * @param line 입력 라인(원본 변경 없음)
 * @return
 * - 0 이상: 선행 탭 개수
 * - -1: 선행 공백이 있어 들여쓰기 오류로 간주
 */
int countIndent(const std::string& line);
/**
 * @brief 라인이 공백(whitespace)만으로 구성되어 있는지 판별한다.
 *
 * @details 모든 문자가 isspace()에 해당하면 true를 반환한다.
 *
 * @param line 입력 라인
 * @return 공백/탭 등 whitespace만 있거나 빈 문자열이면 true, 그 외 문자가 있으면 false
 */
bool isBlankLine(const std::string& line);
/**
 * @brief 문자열을 구분자(delim) 기준으로 분리한다.
 *
 * @details
 * - 연속된 구분자는 무시된다(빈 토큰을 만들지 않음).
 * - 선행/후행 구분자로 인해 생기는 빈 토큰도 만들지 않는다.
 *
 * @param line 입력 문자열
 * @param delim 구분자 문자
 * @return 분리된 토큰 벡터
 */
std::vector<std::string> ftSplit(const std::string& line, char delim);
/**
 * @brief 문자열이 10진수 숫자(0~9)로만 구성되었는지 검사한다.
 *
 * @param s 검사할 문자열
 * @return 비어있지 않고 모든 문자가 숫자면 true, 아니면 false
 * @note 부호(+/-)는 허용하지 않는다. (isdigit에서 걸러줌. 0~9만 봄)
 */
bool isNumber(const std::string& s);
/**
 * @brief 숫자 문자열을 unsigned int로 변환한다.
 *
 * @details
 * - isNumber()로 숫자 문자열임을 확인한 뒤 stringstream으로 변환한다.
 * - 변환 후 스트림에 남는 문자가 있으면 실패한다.
 *
 * @param s 입력 문자열(숫자)
 * @param value [out] 변환 결과
 * @return 변환 성공 시 true, 실패 시 false
 */
bool toInt (const std::string& s, size_t& value);

# endif