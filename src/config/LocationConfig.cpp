#include "../../include/LocationConfig.hpp"

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
 * @param s 변환할 메서드 문자열
 * @param out 변환 성공 시 결과가 저장될 HttpMethod 출력 인자
 * @return 변환 성공 시 true, 허용되지 않은 문자열이면 false
 */
static bool parseHttpMethod(const std::string &s, HttpMethod &out)
{
	if (s == "GET") 
	{
		out = METHOD_GET;
		return true;
	}
	else if (s == "POST")
	{
		out = METHOD_POST;
		return true;
	}
	else if (s == "DELETE")
	{
		out = METHOD_DELETE;
		return true;
	}
	return false;
}

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
bool LocationConfig::parseLocDir(std::vector<std::string> token)
{
	if (token[0] == "root")
	{
		if (token.size() != 2)
			return false;
		
		if (!isValidUriPath(token[1]))
			return false;
		root = token[1];
	}
	else if (token[0] == "index")
	{
		if (token.size() != 2)
			return false;
		
		if (!isValidUriPath(token[1]))
			return false;
		index = token[1];
	}
	else if (token[0] == "methods")
	{

		if (token.size() < 2)
			return false;
		
		methods.clear();
		for (size_t i = 1; i < token.size(); ++i)
		{
			HttpMethod method;
			if (!parseHttpMethod(token[i], method))
				return false;
			methods.insert(method);
		}
	}
	else if (token[0] == "autoindex")
	{
		if (token.size() != 2)
			return false;

		if (token[1] == "on")
			autoIndex = true;
		else if (token[1] == "off")
			autoIndex = false;
		else
			return false;
	}
	else if (token[0] == "upload_dir")
	{
		if (token.size() != 2)
			return false;

		if (!isValidUriPath(token[1]))
			return false;
		uploadDir = token[1];
	}
	else if (token[0] == "return")
	{
		if (token.size() != 2)
			return false;

		if (!isValidUriPath(token[1]))
			return false;
		redirectPath = token[1];
	}
	else if (token[0] == "cgi_ext")
	{
		if (token.size() != 3)
			return false;

		if (!isValidUriPath(token[2]))
			return false;
		cgiExtension = token[1];
		cgiPath = token[2];
	}
	else
		return false;

	return true;
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
LocationConfig::LocationConfig(std::ifstream &configFile)
{
	//변수 초기화
	autoIndex = false;
	methods.insert(METHOD_GET); //메서드 추가할때 clear로 꼭 초기화
	methods.insert(METHOD_POST);
	methods.insert(METHOD_DELETE);

	std::string line;

	while (true)
	{
		std::streampos pos = configFile.tellg();
		if (!std::getline(configFile, line))
			break;
		
		if (isBlankLine(line))
			continue;
		
		int indent = countIndent(line);
		
		if (indent == 0 || indent == 1)
		{
			configFile.clear();
			configFile.seekg(pos);
			status = true;
			return ;
		}
		if (indent == -1 || indent != 2) //indent -1은 countIndent의 에러처리
		{
			status = false;
			return ;
		}
		
		removeIndent(line, '\t');
		std::vector<std::string> token = ftSplit(line, ' ');
		if (token.empty())
		{
			status = false;
			return ;
		}
		if (!parseLocDir(token))
		{
			status = false;
			return ;
		}
	}
	status = false;
	return ;
}