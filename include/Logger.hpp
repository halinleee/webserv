#ifndef LOGGER_HPP
# define LOGGER_HPP

#include <string>
#include <ctime>
#include <iostream>
#include <fstream>

enum LogLevel
{
    LOG_ACCESS,
    LOG_ERROR
};

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

