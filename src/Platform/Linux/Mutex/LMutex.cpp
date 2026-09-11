#include <Winux/Platform/Linux/Linux.h>

#include <cerrno>
#include <cstdint>
#include <fcntl.h>
#include <filesystem>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <sys/file.h>
#include <unistd.h>
#include <utility>

namespace Winux::Platform::Linux {

namespace {

std::string ErrorMessage(const char* operation, const int error)
{
    return std::string(operation) + " failed (error " + std::to_string(error) + ")";
}

std::string MutexFileName(const std::wstring& name)
{
    std::uint64_t hash = 14695981039346656037ull;
    for (const wchar_t character : name)
    {
        hash ^= static_cast<std::uint64_t>(character);
        hash *= 1099511628211ull;
    }

    std::ostringstream stream;
    stream << "mutex-" << std::hex << std::setw(16) << std::setfill('0') << hash << ".lock";
    return stream.str();
}

class LinuxMutex final : public Contracts::IMutex
{
public:
    explicit LinuxMutex(const int descriptor)
        : descriptor_(descriptor)
    {
    }

    ~LinuxMutex() override
    {
        release();
        if (descriptor_ != -1)
        {
            close(descriptor_);
        }
    }

    Core::Result<bool> try_acquire() override
    {
        if (descriptor_ == -1)
        {
            return Core::Result<bool>::failure("Mutex descriptor is invalid");
        }

        if (owns_lock_)
        {
            return Core::Result<bool>::success(true);
        }

        if (flock(descriptor_, LOCK_EX | LOCK_NB) == 0)
        {
            owns_lock_ = true;
            return Core::Result<bool>::success(true);
        }

        const int error = errno;
        if (error == EWOULDBLOCK || error == EAGAIN)
        {
            return Core::Result<bool>::success(false);
        }

        return Core::Result<bool>::failure(ErrorMessage("flock", error));
    }

    Core::Result<void> release() override
    {
        if (descriptor_ == -1 || !owns_lock_)
        {
            return Core::Result<void>::success();
        }

        if (flock(descriptor_, LOCK_UN) != 0)
        {
            return Core::Result<void>::failure(ErrorMessage("flock", errno));
        }

        owns_lock_ = false;
        return Core::Result<void>::success();
    }

    bool owns_lock() const noexcept override
    {
        return owns_lock_;
    }

private:
    int descriptor_ = -1;
    bool owns_lock_ = false;
};

}

Core::Result<std::unique_ptr<Contracts::IMutex>> Linux::create_mutex(
    const std::wstring& name)
{
    if (name.empty())
    {
        return Core::Result<std::unique_ptr<Contracts::IMutex>>::failure(
            "Mutex name cannot be empty");
    }

    const auto temporary = temp();
    if (temporary.failed())
    {
        return Core::Result<std::unique_ptr<Contracts::IMutex>>::failure(temporary.message());
    }

    const std::filesystem::path directory = temporary.value() / "Winux" / "Mutexes";
    std::error_code directory_error;
    std::filesystem::create_directories(directory, directory_error);
    if (directory_error)
    {
        return Core::Result<std::unique_ptr<Contracts::IMutex>>::failure(
            "Unable to create mutex directory: " + directory_error.message());
    }

    const auto file = directory / MutexFileName(name);
    const int descriptor = open(file.c_str(), O_RDWR | O_CREAT, 0600);
    if (descriptor == -1)
    {
        return Core::Result<std::unique_ptr<Contracts::IMutex>>::failure(
            ErrorMessage("open", errno));
    }

    return Core::Result<std::unique_ptr<Contracts::IMutex>>::success(
        std::make_unique<LinuxMutex>(descriptor));
}

}
