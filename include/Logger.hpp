#ifndef LOGGER_HPP
# define LOGGER_HPP

#include <string>
#include <ctime>
#include <iostream>
#include <fstream>

/**
 * @brief 로그의 종류를 구분하는 enum
 * @var LOG_ACCESS 정상 요청-응답 처리 결과를 기록 (access.log)
 * @var LOG_ERROR 서버 내부 오류를 기록 (error.log)
 */
enum LogLevel
{
    LOG_ACCESS,
    LOG_ERROR
};

/**
 * @brief 로그 한 줄을 기록하고 바로 소멸되는 RAII 로거
 * @details 생성자에서 nginx 스타일 타임스탬프를 붙여 콘솔과 로그 파일에 즉시 기록하고,
 * 소멸자에서 파일을 닫는다. 상태를 유지하지 않으므로 매 로그 호출마다 임시 객체로 생성해서 쓴다.
 *
 * 사용 예)
 * @code
 * Logger(LOG_ERROR, "Client Accept Failed");
 * @endcode
 */
class Logger
{
    private:
        std::ofstream logFile;

        std::string getTimeLocal(void) const;
        std::string accesLogBuild() const; 
        const std::string getLogPath(LogLevel level) const;
        void writeLine(LogLevel level, const std::string &line);

    public:
        Logger(LogLevel level, const std::string &message);
        Logger(LogLevel level, const std::string &message, const std::string &ip);
        ~Logger();
};

#endif
