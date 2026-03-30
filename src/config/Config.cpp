#include "../../include/Config.hpp"

// bool Config::locationValidate(const std::vector<std::string>& token)
// {

// }

bool Config::errorPageValidate(unsigned int key, const std::vector<std::string>& token)
{
	if (token.size() != 3)
		return (false);

	unsigned int num = 0;
	if (!toInt(token[1], num))
		return (false);
	
	//하륀스와 얘기할것
	//에러코드 처리 1. 없는 에러코드에 대해서 config 쪽에서는 그냥 숫자면 담고 요청부분에서 에러코드 거를것인지
	//에러코드 처리 2. 에러코드를 한정적으로 가능한것만 정리해서 config에서 거를것인지
	//에러페이지 경로 처리
	//실제로 존재하는 경로인지는 config 파싱쪽에서는 실질적으로 확인하기 불가
	//1. config쪽에서는 문자열이 존재하는지 확인만 하고 담고 요청에서 실질적으로 존재하는지 유효성 검사로 거를 것인지
	//2. config에서 / 단위로 토큰이 존재하는지 정도 확인할 수 있을듯 
	//우짜면 좋을까나~~
	servers[key].setErrorPages(num, token[2]);
	return (true);
}


bool Config::listenBodyValidate(unsigned int key, const std::vector<std::string>& token, size_t max)
{
	if (token.size() != 2)
		return (false);

	unsigned int num = 0;
	if (!toInt(token[1], num))
		return (false);

	if (num == 0 || num > max)
		return (false);

	if (token[0] == "client_max_body_size")
		servers[key].setClientMaxBodySize(static_cast<size_t>(num));
	return (true);
}



/**
 * @struct serverDirectiveValidate
 * @brief 서버 설정 지시어들의 유효 검사를 진입하는 함수
 */
bool Config::serverDirectiveValidate(unsigned int key, const std::vector<std::string>& token)
{	
	if (token[0] == "client_max_body_size")
		return (listenBodyValidate(key, token, 10000000));
	else if (token[0] == "error_page")
		return (errorPageValidate(key, token));
	// else if (token[0] == "location")//로케이션 검사 다끝내기
	// 	return (locationValidate(token));
	return (false);
}

/**
 * @struct validateConfig
 * @brief configfile 문법 유효성 검사 함수
 * 
 * 오타, 블록 단위, 지시어 등 문법이 유효한지 검사하여
 * 유효한 configfile이면 true
 * 오류있는 configfile이면 false를 반환한다
 */
int Config::validateConfig(std::ifstream& configFile)
{
	std::string configLine;

	while (std::getline(configFile, configLine))
	{
		if (isBlankLine(configLine))
			continue;
		else
			break;
	}
	if (configFile.eof())//정상적으로 더읽을 데이터 없음
		return (1);
	
	std::vector<std::string> token = ftSplit(configLine, ' ');

	if (token[0] != "server" || !listenBodyValidate(0, token, 9999))
	{
		std::cout << "Config error: server or port error" << std::endl;
		std::cout << "error line: " << configLine << std::endl;
		return (-1);
	}

	unsigned int key = 0;
	toInt(token[1], key);
	servers[key]; //port번호 key값 등록
		
	while (std::getline(configFile, configLine))
	{
		if (isBlankLine(configLine))
			continue;

		int indent = countIndent(configLine);
		
		if (indent == 0 && configLine == "end")//server가 나왔을때만 end허용하게
			return (0);
		
		if (indent != 1)
		{
			std::cout << "Config error: indent error" << std::endl;
			std::cout << "error line: " << configLine << std::endl;
			return (-1);
		}

		removeChar(configLine, '\t');
		token = ftSplit(configLine, ' ');
		if (!serverDirectiveValidate(key, token)) //라인 끝에 공백 많을때 공백 처리(toInt에서 공백을 어떻게 처리하는가)
			return (-1);
	}
	return (-1);
	// return (1); //모든 서버 검사를 마쳤을때
	// return (0); //한개의 서버 검사를 마쳤을때
	// return (-1); //에러
}


// configFile.clear();
// configFile.seekg(0, std::ios::beg);