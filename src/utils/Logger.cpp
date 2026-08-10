#include "Logger.hpp"

std::string Logger::getTimeLocal(void) const
{
    time_t now = time(NULL);
    struct tm localTm;
    localtime_r(&now, &localTm);

    char buf[64];
    strftime(buf, sizeof(buf), "[%d/%b/%Y:%H:%M:%S %z]", &localTm);
    return std::string(buf);
}


const std::string Logger::getLogPath(LogLevel level) const
{
    if (level == LOG_ACCESS)
        return "./access.log";
    else
        return "./error.log";
}


void Logger::writeLine(LogLevel level, const std::string &line)
{
    if (level == LOG_ACCESS)
        std::cout << line << std::endl;
    else
        std::cerr << line << std::endl;

    if (logFile.is_open())
        logFile << line << std::endl;
}

Logger::Logger(LogLevel level, const std::string &message)
    : logFile(getLogPath(level).c_str(), std::ios::app)
{
    writeLine(level, getTimeLocal() + " " + message);
}

Logger::Logger(LogLevel level, const std::string &message, const std::string &ip)
    : logFile(getLogPath(level).c_str(), std::ios::app)
{
    writeLine(level, ip + " - - " + getTimeLocal() + " " + message);
}


Logger::~Logger()
{
    if (logFile.is_open())
        logFile.close();
}