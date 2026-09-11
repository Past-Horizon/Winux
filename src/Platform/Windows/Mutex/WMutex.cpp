#include <Winux/Platform/Windows/Win32.h>

#include <windows.h>

#include <filesystem>
#include <cstdint>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <utility>

namespace Winux::Platform::Windows {

namespace {

std::string ErrorMessage(const char* operation, const DWORD error)
{
    return std::string(operation) + " failed (error " + std::to_string(error) + ")";
}

std::wstring MutexFileName(const std::wstring& name)
{
    std::uint64_t hash = 14695981039346656037ull;
    for (const wchar_t character : name)
    {
        hash ^= static_cast<std::uint64_t>(character);
        hash *= 1099511628211ull;
    }

    std::wostringstream stream;
    stream << L"mutex-" << std::hex << std::setw(16) << std::setfill(L'0') << hash << L".lock";
    return stream.str();
}

class WindowsMutex final : public Contracts::IMutex
{
public:
    explicit WindowsMutex(HANDLE handle)
        : handle_(handle)
    {
    }

    ~WindowsMutex() override
    {
        release();
        if (handle_ != INVALID_HANDLE_VALUE)
        {
            CloseHandle(handle_);
        }
    }

    Core::Result<bool> try_acquire() override
    {
        if (handle_ == INVALID_HANDLE_VALUE)
        {
            return Core::Result<bool>::failure("Mutex handle is invalid");
        }

        if (owns_lock_)
        {
            return Core::Result<bool>::success(true);
        }

        OVERLAPPED overlapped{};
        if (LockFileEx(
                handle_,
                LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY,
                0,
                1,
                0,
                &overlapped))
        {
            owns_lock_ = true;
            return Core::Result<bool>::success(true);
        }

        const DWORD error = GetLastError();
        if (error == ERROR_LOCK_VIOLATION)
        {
            return Core::Result<bool>::success(false);
        }

        return Core::Result<bool>::failure(ErrorMessage("LockFileEx", error));
    }

    Core::Result<void> release() override
    {
        if (handle_ == INVALID_HANDLE_VALUE || !owns_lock_)
        {
            return Core::Result<void>::success();
        }

        OVERLAPPED overlapped{};
        if (!UnlockFileEx(handle_, 0, 1, 0, &overlapped))
        {
            return Core::Result<void>::failure(ErrorMessage("UnlockFileEx", GetLastError()));
        }

        owns_lock_ = false;
        return Core::Result<void>::success();
    }

    bool owns_lock() const noexcept override
    {
        return owns_lock_;
    }

private:
    HANDLE handle_ = INVALID_HANDLE_VALUE;
    bool owns_lock_ = false;
};

}

Core::Result<std::unique_ptr<Contracts::IMutex>> Win32::create_mutex(
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
    const HANDLE handle = CreateFileW(
        file.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);
    if (handle == INVALID_HANDLE_VALUE)
    {
        return Core::Result<std::unique_ptr<Contracts::IMutex>>::failure(
            ErrorMessage("CreateFileW", GetLastError()));
    }

    return Core::Result<std::unique_ptr<Contracts::IMutex>>::success(
        std::make_unique<WindowsMutex>(handle));
}

}
