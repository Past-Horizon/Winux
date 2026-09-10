#include <sstream>
#include <string>
#include <utility>

namespace Logger
{
    enum class Level
    {
        Info,
        Warn,
        Error
    };

    void Log(Logger::Level level, const std::string& msg);

    template <typename... Args>
    void Log(Logger::Level level, Args&&... args)
    {
        std::ostringstream message;
        (message << ... << std::forward<Args>(args));
        const std::string composed_message = message.str();
        Log(level, composed_message);
    }
}