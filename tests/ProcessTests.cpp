#include <Winux/Winux.h>
#include "TestPlatform.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace {

class ProcessTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        Winux::Testing::InitializePlatformOnce();
        platform = &Winux::Get<Winux::PlatformContext>();
        process = &Winux::Get<Winux::Process>();
    }

    void TearDown() override
    {
        for (const std::uint32_t process_id : created_processes)
        {
            const auto running = process->IsRunning(process_id);
            if (running.Succeeded() && running.Value())
            {
                process->ForceTerminateProcess(process_id);
            }
        }
    }

    std::uint32_t create_test_process()
    {
#ifdef _WIN32
        const auto result = process->CreateProcess(
            L"cmd.exe /c \"ping 127.0.0.1 -n 30 > nul\"");
#else
        const auto result = process->CreateProcess(L"sleep 30");
#endif
        EXPECT_TRUE(result.Succeeded()) << result.Message();
        if (result.Failed())
        {
            return 0;
        }

        created_processes.push_back(result.Value());
        return result.Value();
    }

    Winux::PlatformContext* platform = nullptr;
    Winux::Process* process = nullptr;
    std::vector<std::uint32_t> created_processes;
};

TEST_F(ProcessTests, PlatformCreationProvidesProcessApi)
{
    ASSERT_NE(platform, nullptr);
    ASSERT_NE(process, nullptr);
}

TEST_F(ProcessTests, ProcessCapabilitiesMatchPlatformSupport)
{
    const auto supported = platform->SupportedFeatures();
    EXPECT_TRUE(supported.Has<Winux::Process::Detached>());
#ifdef _WIN32
    EXPECT_TRUE(supported.Has<Winux::Process::CreateNoWindow>());
    EXPECT_TRUE(supported.Has<Winux::Process::CreateNewConsole>());
#else
    EXPECT_FALSE(supported.Has<Winux::Process::CreateNoWindow>());
    EXPECT_FALSE(supported.Has<Winux::Process::CreateNewConsole>());
#endif
}

TEST_F(ProcessTests, CreateProcessReturnsRunningProcess)
{
    const std::uint32_t process_id = create_test_process();
    ASSERT_NE(process_id, 0u);

    const auto running = process->IsRunning(process_id);
    ASSERT_TRUE(running.Succeeded()) << running.Message();
    EXPECT_TRUE(running.Value());
}

TEST_F(ProcessTests, DetachedCapabilityStartsProcess)
{
    Winux::CapabilitySet options;
    options.Add<Winux::Process::Detached>();
#ifdef _WIN32
    const auto result = process->CreateProcess(
        L"cmd.exe /c \"ping 127.0.0.1 -n 30 > nul\"",
        options);
#else
    const auto result = process->CreateProcess(
        L"sleep 30", options);
#endif
    ASSERT_TRUE(result.Succeeded()) << result.Message();
    created_processes.push_back(result.Value());

    const auto running = process->IsRunning(result.Value());
    ASSERT_TRUE(running.Succeeded()) << running.Message();
    EXPECT_TRUE(running.Value());
}

TEST_F(ProcessTests, FindLocationReturnsExecutablePath)
{
    const std::uint32_t process_id = create_test_process();
    ASSERT_NE(process_id, 0u);

    const auto location = process->FindLocation(process_id);
    ASSERT_TRUE(location.Succeeded()) << location.Message();
    EXPECT_TRUE(std::filesystem::exists(location.Value()));
    EXPECT_FALSE(location.Value().empty());
}

TEST_F(ProcessTests, FindLocationRejectsInvalidProcess)
{
    const auto location = process->FindLocation(0);
    EXPECT_TRUE(location.Failed());
}

TEST_F(ProcessTests, GetExecutableDirectoryReturnsCurrentDirectory)
{
    const auto directory = process->GetExecutableDirectory();
    ASSERT_TRUE(directory.Succeeded()) << directory.Message();
    EXPECT_TRUE(std::filesystem::is_directory(directory.Value()));
}

TEST_F(ProcessTests, GetExecutableDirectoryReturnsProcessDirectory)
{
    const std::uint32_t process_id = create_test_process();
    ASSERT_NE(process_id, 0u);

    const auto directory = process->GetExecutableDirectory(process_id);
    ASSERT_TRUE(directory.Succeeded()) << directory.Message();
    EXPECT_TRUE(std::filesystem::is_directory(directory.Value()));
}

TEST_F(ProcessTests, FindProcessFindsCreatedProcess)
{
    const std::uint32_t process_id = create_test_process();
    ASSERT_NE(process_id, 0u);

#ifdef _WIN32
    const auto result = process->FindProcess(L"cmd.exe");
#else
    const auto result = process->FindProcess(L"sleep");
#endif
    ASSERT_TRUE(result.Succeeded()) << result.Message();
    ASSERT_TRUE(result.Value().has_value());
}

TEST_F(ProcessTests, FindProcessReportsMissingProcess)
{
    const auto result = process->FindProcess(L"winux-process-that-does-not-exist");
    ASSERT_TRUE(result.Succeeded()) << result.Message();
    EXPECT_FALSE(result.Value().has_value());
}

TEST_F(ProcessTests, FindProcessesReturnsCreatedProcess)
{
    const std::uint32_t process_id = create_test_process();
    ASSERT_NE(process_id, 0u);

#ifdef _WIN32
    const auto result = process->FindProcesses(L"cmd.exe");
#else
    const auto result = process->FindProcesses(L"sleep");
#endif
    ASSERT_TRUE(result.Succeeded()) << result.Message();
    EXPECT_FALSE(result.Value().empty());
}

TEST_F(ProcessTests, IsRunningChangesAfterTermination)
{
    const std::uint32_t process_id = create_test_process();
    ASSERT_NE(process_id, 0u);

    const auto before = process->IsRunning(process_id);
    ASSERT_TRUE(before.Succeeded()) << before.Message();
    ASSERT_TRUE(before.Value());

    const auto terminated = process->ForceTerminateProcess(process_id);
    ASSERT_TRUE(terminated.Succeeded()) << terminated.Message();

    const auto after = process->IsRunning(process_id);
    ASSERT_TRUE(after.Succeeded()) << after.Message();
    EXPECT_FALSE(after.Value());
}

TEST_F(ProcessTests, TerminateRejectsInvalidProcess)
{
    const auto terminated = process->TerminateProcess(0);
    EXPECT_TRUE(terminated.Failed());
}

TEST_F(ProcessTests, ForceTerminateRejectsInvalidProcess)
{
    const auto terminated = process->ForceTerminateProcess(0);
    EXPECT_TRUE(terminated.Failed());
}

#ifdef _WIN32
TEST_F(ProcessTests, NoWindowCapabilityStartsProcess)
{
    Winux::CapabilitySet options;
    options.Add<Winux::Process::CreateNoWindow>();
    const auto result = process->CreateProcess(
        L"cmd.exe /c \"ping 127.0.0.1 -n 30 > nul\"",
        options);
    ASSERT_TRUE(result.Succeeded()) << result.Message();
    created_processes.push_back(result.Value());

    const auto running = process->IsRunning(result.Value());
    ASSERT_TRUE(running.Succeeded()) << running.Message();
    EXPECT_TRUE(running.Value());
}

TEST_F(ProcessTests, NewConsoleCapabilityStartsProcess)
{
    Winux::CapabilitySet options;
    options.Add<Winux::Process::CreateNewConsole>();
    const auto result = process->CreateProcess(
        L"cmd.exe /c \"ping 127.0.0.1 -n 30 > nul\"",
        options);
    ASSERT_TRUE(result.Succeeded()) << result.Message();
    created_processes.push_back(result.Value());

    const auto running = process->IsRunning(result.Value());
    ASSERT_TRUE(running.Succeeded()) << running.Message();
    EXPECT_TRUE(running.Value());
}

TEST_F(ProcessTests, DetachedAndNewConsoleCapabilitiesAreIncompatible)
{
    Winux::CapabilitySet options;
    options.Add<Winux::Process::Detached>();
    options.Add<Winux::Process::CreateNewConsole>();
    const auto result = process->CreateProcess(
        L"cmd.exe /c \"ping 127.0.0.1 -n 30 > nul\"", options);
    EXPECT_TRUE(result.Failed());
}
#else
TEST_F(ProcessTests, UnsupportedWindowsCapabilitiesFailBeforeStarting)
{
    Winux::CapabilitySet no_window_options;
    no_window_options.Add<Winux::Process::CreateNoWindow>();
    const auto no_window = process->CreateProcess(L"sleep 30", no_window_options);
    EXPECT_TRUE(no_window.Failed());

    Winux::CapabilitySet new_console_options;
    new_console_options.Add<Winux::Process::CreateNewConsole>();
    const auto new_console = process->CreateProcess(L"sleep 30", new_console_options);
    EXPECT_TRUE(new_console.Failed());
}
#endif

} // namespace
