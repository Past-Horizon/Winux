#include <sstream>
#include <string>
#include <utility>

namespace Logger
{
    /*
        @summary
        Severity used when writing diagnostic output.
    */
    enum class Level
    {
        Info,
        Warn,
        Error
    };

    /*
        @summary
        Writes a message using the provided log severity.

        @param level
        Severity assigned to the message.

        @param msg
        Text content to emit.
    */
    void Log(Logger::Level level, const std::string& msg);

    /*
        @summary
        Formats any number of values into a single message and logs it.

        @param level
        Severity assigned to the message.

        @param args
        Values to concatenate into the final log text.
    */
    template <typename... Args>
    void Log(Logger::Level level, Args&&... args)
    {
        std::ostringstream message;
        (message << ... << std::forward<Args>(args));
        const std::string composed_message = message.str();
        Log(level, composed_message);
    }
}