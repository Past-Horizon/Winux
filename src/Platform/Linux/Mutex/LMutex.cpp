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
    return std::string(operation) + " Failed (error " + std::to_string(error) + ")";
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
        Release();
        if (descriptor_ != -1)
        {
            close(descriptor_);
        }
    }

    Core::Result<bool> TryAcquire() override
    {
        if (descriptor_ == -1)
        {
            return Core::Result<bool>::Failure("Mutex descriptor is invalid");
        }

        if (owns_lock_)
        {
            return Core::Result<bool>::Success(true);
        }

        if (flock(descriptor_, LOCK_EX | LOCK_NB) == 0)
        {
            owns_lock_ = true;
            return Core::Result<bool>::Success(true);
        }

        const int error = errno;
        if (error == EWOULDBLOCK || error == EAGAIN)
        {
            return Core::Result<bool>::Success(false);
        }

        return Core::Result<bool>::Failure(ErrorMessage("flock", error));
    }

    Core::Result<void> Release() override
    {
        if (descriptor_ == -1 || !owns_lock_)
        {
            return Core::Result<void>::Success();
        }

        if (flock(descriptor_, LOCK_UN) != 0)
        {
            return Core::Result<void>::Failure(ErrorMessage("flock", errno));
        }

        owns_lock_ = false;
        return Core::Result<void>::Success();
    }

    bool OwnsLock() const noexcept override
    {
        return owns_lock_;
    }

private:
    int descriptor_ = -1;
    bool owns_lock_ = false;
};

}

Core::Result<std::unique_ptr<Contracts::IMutex>> Linux::CreateMutex(
    const std::wstring& name)
{
    if (name.empty())
    {
        return Core::Result<std::unique_ptr<Contracts::IMutex>>::Failure(
            "Mutex name cannot be empty");
    }

    const auto temporary = Temp();
    if (temporary.Failed())
    {
        return Core::Result<std::unique_ptr<Contracts::IMutex>>::Failure(temporary.Message());
    }

    const std::filesystem::path directory = temporary.Value() / "Winux" / "Mutexes";
    std::error_code directory_error;
    std::filesystem::create_directories(directory, directory_error);
    if (directory_error)
    {
        return Core::Result<std::unique_ptr<Contracts::IMutex>>::Failure(
            "Unable to create mutex directory: " + directory_error.message());
    }

    const auto file = directory / MutexFileName(name);
    const int descriptor = open(file.c_str(), O_RDWR | O_CREAT, 0600);
    if (descriptor == -1)
    {
        return Core::Result<std::unique_ptr<Contracts::IMutex>>::Failure(
            ErrorMessage("open", errno));
    }

    return Core::Result<std::unique_ptr<Contracts::IMutex>>::Success(
        std::make_unique<LinuxMutex>(descriptor));
}

}
