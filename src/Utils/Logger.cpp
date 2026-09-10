#include <iostream>
#include <Winux/Utils/Logger.h>

namespace Logger
{
    std::string ToString(Logger::Level level)
    {
        switch (level)
        {
            case Logger::Level::Info: return "INFO ";
            case Logger::Level::Warn: return "WARN ";
            case Logger::Level::Error: return "ERROR ";
        }

        return "";
    }

    void Log(Logger::Level level, const std::string& msg)
    {
        std::cout << ToString(level) << msg << '\n';
    }
}