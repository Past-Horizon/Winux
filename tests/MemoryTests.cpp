#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#include <Winux/Winux.h>
#include "TestPlatform.h"

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#ifndef _WIN32
#include <cerrno>
#endif

namespace {

class MemoryTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        Winux::Testing::InitializePlatformOnce();
        memory = &Winux::Get<Winux::Memory>();
        process = &Winux::Get<Winux::Process>();
    }

    void TearDown() override
    {
#ifdef _WIN32
        if (remote_buffer != nullptr && remote_process != nullptr)
        {
            VirtualFreeEx(remote_process, remote_buffer, 0, MEM_RELEASE);
        }
        if (remote_process != nullptr)
        {
            CloseHandle(remote_process);
        }
        if (child_process_id != 0)
        {
            const auto running = process->IsRunning(child_process_id);
            if (running.Succeeded() && running.Value())
            {
                process->ForceTerminateProcess(child_process_id);
            }
        }
#else
        if (child_release_pipe != -1)
        {
            const char release = 1;
            ssize_t written = -1;
            do
            {
                written = write(child_release_pipe, &release, sizeof(release));
            } while (written == -1 && errno == EINTR);
            close(child_release_pipe);
        }
        if (child_process_id > 0)
        {
            int status = 0;
            while (waitpid(child_process_id, &status, 0) == -1 && errno == EINTR)
            {
            }
        }
        if (remote_buffer != MAP_FAILED)
        {
            munmap(remote_buffer, 4096);
        }
#endif
    }

    Winux::Memory* memory = nullptr;
    Winux::Process* process = nullptr;
#ifdef _WIN32
    std::uint32_t child_process_id = 0;
    HANDLE remote_process = nullptr;
    void* remote_buffer = nullptr;
#else
    pid_t child_process_id = -1;
    int child_release_pipe = -1;
    void* remote_buffer = MAP_FAILED;
#endif
};

TEST_F(MemoryTests, ReadsAndWritesCurrentProcessMemory)
{
    const auto current_process_id = process->GetCurrentProcessId();
    ASSERT_TRUE(current_process_id.Succeeded()) << current_process_id.Message();
    const std::uint32_t process_id = current_process_id.Value();
    std::array<std::byte, 4> target{};
    const std::array<std::byte, 4> expected{
        std::byte{0x12}, std::byte{0x34}, std::byte{0x56}, std::byte{0x78}};

    const auto written = memory->WriteMemory(
        process_id,
        reinterpret_cast<std::uintptr_t>(target.data()),
        expected);
    ASSERT_TRUE(written.Succeeded()) << written.Message();
    EXPECT_EQ(written.Value(), expected.size());
    EXPECT_EQ(target, expected);

    std::array<std::byte, 4> actual{};
    const auto read = memory->ReadMemory(
        process_id,
        reinterpret_cast<std::uintptr_t>(target.data()),
        actual);
    ASSERT_TRUE(read.Succeeded()) << read.Message();
    EXPECT_EQ(read.Value(), actual.size());
    EXPECT_EQ(actual, expected);
}

TEST_F(MemoryTests, ReadsAndWritesSeparateProcessMemory)
{
#ifdef _WIN32
    const auto created = process->CreateProcess(
        L"cmd.exe /c \"ping 127.0.0.1 -n 30 > nul\"");
    ASSERT_TRUE(created.Succeeded()) << created.Message();
    child_process_id = created.Value();

    remote_process = OpenProcess(
        PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE,
        FALSE,
        child_process_id);
    ASSERT_NE(remote_process, nullptr) << GetLastError();

    remote_buffer = VirtualAllocEx(
        remote_process,
        nullptr,
        4096,
        MEM_RESERVE | MEM_COMMIT,
        PAGE_READWRITE);
    ASSERT_NE(remote_buffer, nullptr) << GetLastError();

    const std::array<std::byte, 4> expected{
        std::byte{0x9a}, std::byte{0xbc}, std::byte{0xde}, std::byte{0xf0}};
    const auto written = memory->WriteMemory(
        child_process_id,
        reinterpret_cast<std::uintptr_t>(remote_buffer),
        expected);
    ASSERT_TRUE(written.Succeeded()) << written.Message();
    EXPECT_EQ(written.Value(), expected.size());

    std::array<std::byte, 4> actual{};
    const auto read = memory->ReadMemory(
        child_process_id,
        reinterpret_cast<std::uintptr_t>(remote_buffer),
        actual);
    ASSERT_TRUE(read.Succeeded()) << read.Message();
    EXPECT_EQ(read.Value(), actual.size());
    EXPECT_EQ(actual, expected);
#else
    int release_pipe[2]{};
    ASSERT_EQ(pipe(release_pipe), 0) << errno;

    remote_buffer = mmap(
        nullptr,
        4096,
        PROT_READ | PROT_WRITE,
        MAP_SHARED | MAP_ANONYMOUS,
        -1,
        0);
    ASSERT_NE(remote_buffer, MAP_FAILED) << errno;

    child_process_id = fork();
    if (child_process_id < 0)
    {
        const int error = errno;
        close(release_pipe[0]);
        close(release_pipe[1]);
        munmap(remote_buffer, 4096);
        remote_buffer = MAP_FAILED;
        FAIL() << error;
    }
    if (child_process_id == 0)
    {
        close(release_pipe[1]);

        char release = 0;
        ssize_t released = -1;
        do
        {
            released = read(release_pipe[0], &release, sizeof(release));
        } while (released == -1 && errno == EINTR);
        close(release_pipe[0]);
        _exit(released == 1 ? 0 : 1);
    }

    close(release_pipe[0]);
    child_release_pipe = release_pipe[1];
    const std::uintptr_t remote_address = reinterpret_cast<std::uintptr_t>(remote_buffer);

    const std::array<std::byte, 4> expected{
        std::byte{0x9a}, std::byte{0xbc}, std::byte{0xde}, std::byte{0xf0}};
    const auto written = memory->WriteMemory(
        static_cast<std::uint32_t>(child_process_id),
        remote_address,
        expected);
    ASSERT_TRUE(written.Succeeded()) << written.Message();
    EXPECT_EQ(written.Value(), expected.size());

    std::array<std::byte, 4> actual{};
    const auto read = memory->ReadMemory(
        static_cast<std::uint32_t>(child_process_id),
        remote_address,
        actual);
    ASSERT_TRUE(read.Succeeded()) << read.Message();
    EXPECT_EQ(read.Value(), actual.size());
    EXPECT_EQ(actual, expected);
#endif
}

TEST_F(MemoryTests, EmptyBuffersSucceedWithoutValidatingProcess)
{
    std::span<std::byte> output;
    std::span<const std::byte> input;

    const auto read = memory->ReadMemory(0, 0, output);
    ASSERT_TRUE(read.Succeeded()) << read.Message();
    EXPECT_EQ(read.Value(), 0u);

    const auto written = memory->WriteMemory(0, 0, input);
    ASSERT_TRUE(written.Succeeded()) << written.Message();
    EXPECT_EQ(written.Value(), 0u);
}

TEST_F(MemoryTests, RejectsInvalidProcessAndAddress)
{
    std::array<std::byte, 4> buffer{};
    const auto invalid_process = memory->ReadMemory(0, 1, buffer);
    EXPECT_TRUE(invalid_process.Failed());

    const auto current_process_id = process->GetCurrentProcessId();
    ASSERT_TRUE(current_process_id.Succeeded()) << current_process_id.Message();

    const auto invalid_address = memory->WriteMemory(
        current_process_id.Value(),
        0,
        std::span<const std::byte>(buffer));
    EXPECT_TRUE(invalid_address.Failed());
}

#ifndef _WIN32
TEST_F(MemoryTests, RejectsProcessIdOutsidePidRange)
{
    std::array<std::byte, 1> buffer{};
    const auto result = memory->ReadMemory(
        (std::numeric_limits<std::uint32_t>::max)(),
        1,
        buffer);
    EXPECT_TRUE(result.Failed());
    EXPECT_TRUE(memory->WriteMemory(
        (std::numeric_limits<std::uint32_t>::max)(),
        1,
        buffer).Failed());
}

TEST_F(MemoryTests, WritesToReadOnlyCurrentProcessMemory)
{
    const auto current_process_id = process->GetCurrentProcessId();
    ASSERT_TRUE(current_process_id.Succeeded()) << current_process_id.Message();

    auto* page = static_cast<std::byte*>(mmap(
        nullptr,
        4096,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS,
        -1,
        0));
    ASSERT_NE(page, MAP_FAILED) << errno;
    ASSERT_EQ(mprotect(page, 4096, PROT_READ), 0) << errno;

    const std::array<std::byte, 1> expected{ std::byte{ 0x5a } };
    const auto written = memory->WriteMemory(
        current_process_id.Value(),
        reinterpret_cast<std::uintptr_t>(page),
        expected);

    ASSERT_EQ(mprotect(page, 4096, PROT_READ | PROT_WRITE), 0) << errno;
    EXPECT_TRUE(written.Succeeded()) << written.Message();
    EXPECT_EQ(page[0], expected[0]);
    EXPECT_EQ(munmap(page, 4096), 0) << errno;
}
#endif

}