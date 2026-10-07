#include <Winux/Winux.h>
#include "TestPlatform.h"
#ifdef _WIN32
#include <Winux/Platform/Windows/Process/WProcessArchitecture.h>
#include <winnt.h>
#else
#include <Winux/Platform/Linux/Process/LProcessArchitecture.h>
#include <Winux/Platform/Linux/Process/LProcessUtils.h>
#include <elf.h>
#endif

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <optional>
#include <string>
#include <thread>
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

TEST_F(ProcessTests, GetArchitectureReturnsX64ForCurrentProcess)
{
    const auto process_id = process->GetCurrentProcessId();
    ASSERT_TRUE(process_id.Succeeded()) << process_id.Message();

    const auto architecture = process->GetArchitecture(process_id.Value());
    ASSERT_TRUE(architecture.Succeeded()) << architecture.Message();
    EXPECT_EQ(architecture.Value(), Winux::Process::Architecture::X64);
}

TEST_F(ProcessTests, GetArchitectureRejectsInvalidProcess)
{
    const auto architecture = process->GetArchitecture(0);
    EXPECT_TRUE(architecture.Failed());
}

#ifdef _WIN32
TEST_F(ProcessTests, ArchitectureMappingCoversWindowsMachineTypes)
{
    using Architecture = Winux::Process::Architecture;
    using Winux::Platform::Windows::Process::Detail::ArchitectureFromMachine;

    EXPECT_EQ(ArchitectureFromMachine(IMAGE_FILE_MACHINE_I386), Architecture::X86);
    EXPECT_EQ(ArchitectureFromMachine(IMAGE_FILE_MACHINE_AMD64), Architecture::X64);
    EXPECT_EQ(ArchitectureFromMachine(IMAGE_FILE_MACHINE_ARM), Architecture::Arm);
    EXPECT_EQ(ArchitectureFromMachine(IMAGE_FILE_MACHINE_ARMNT), Architecture::Arm);
    EXPECT_EQ(ArchitectureFromMachine(IMAGE_FILE_MACHINE_ARM64), Architecture::Arm64);
    EXPECT_EQ(ArchitectureFromMachine(0xffff), Architecture::Unknown);
}
#else
TEST_F(ProcessTests, ArchitectureMappingCoversLinuxMachineTypes)
{
    using Architecture = Winux::Process::Architecture;
    using Winux::Platform::Linux::Process::Detail::ArchitectureFromElfMachine;

    EXPECT_EQ(ArchitectureFromElfMachine(EM_386, ELFCLASS32, ELFDATA2LSB), Architecture::X86);
    EXPECT_EQ(ArchitectureFromElfMachine(EM_X86_64, ELFCLASS64, ELFDATA2LSB), Architecture::X64);
    EXPECT_EQ(ArchitectureFromElfMachine(EM_ARM, ELFCLASS32, ELFDATA2LSB), Architecture::Arm);
    EXPECT_EQ(ArchitectureFromElfMachine(EM_AARCH64, ELFCLASS64, ELFDATA2LSB), Architecture::Arm64);
    EXPECT_EQ(ArchitectureFromElfMachine(EM_NONE, ELFCLASSNONE, ELFDATA2LSB), Architecture::Unknown);
    EXPECT_EQ(ArchitectureFromElfMachine(EM_X86_64, ELFCLASS32, ELFDATA2LSB), Architecture::Unknown);
    EXPECT_EQ(ArchitectureFromElfMachine(EM_AARCH64, ELFCLASS64, ELFDATA2MSB), Architecture::Unknown);
}
#endif

TEST_F(ProcessTests, GetArchitectureReturnsX64ForCreatedProcess)
{
    const std::uint32_t process_id = create_test_process();
    ASSERT_NE(process_id, 0u);

    const auto architecture = process->GetArchitecture(process_id);
    ASSERT_TRUE(architecture.Succeeded()) << architecture.Message();
    EXPECT_EQ(architecture.Value(), Winux::Process::Architecture::X64);
}

#ifndef _WIN32
TEST_F(ProcessTests, RejectsProcessIdsThatDoNotFitPidType)
{
    constexpr std::uint32_t invalid_process_id =
        (std::numeric_limits<std::uint32_t>::max)();

    EXPECT_FALSE(Winux::Platform::Linux::Process::Detail::IsValidProcessId(
        invalid_process_id));
    EXPECT_TRUE(process->IsRunning(invalid_process_id).Failed());
    EXPECT_TRUE(process->GetArchitecture(invalid_process_id).Failed());
}
#endif
TEST_F(ProcessTests, TerminationRejectsNegativeTimeout)
{
    const std::uint32_t process_id = create_test_process();
    ASSERT_NE(process_id, 0u);

    EXPECT_TRUE(process->TerminateProcess(
        process_id,
        std::chrono::milliseconds{ -1 }).Failed());
    EXPECT_TRUE(process->ForceTerminateProcess(
        process_id,
        std::chrono::milliseconds{ -1 }).Failed());
}

#ifndef _WIN32
TEST_F(ProcessTests, CreateProcessParsesQuotedArguments)
{
    const auto result = process->CreateProcess(L"sh -c \"sleep 30\"");
    ASSERT_TRUE(result.Succeeded()) << result.Message();
    created_processes.push_back(result.Value());

    const auto running = process->IsRunning(result.Value());
    ASSERT_TRUE(running.Succeeded()) << running.Message();
    EXPECT_TRUE(running.Value());
}

TEST_F(ProcessTests, TerminateProcessTimesOutForIgnoredSignal)
{
    const auto result = process->CreateProcess(L"sh -c \"trap '' TERM; exec sleep 30\"");
    ASSERT_TRUE(result.Succeeded()) << result.Message();
    created_processes.push_back(result.Value());

    const auto terminated = process->TerminateProcess(
        result.Value(),
        std::chrono::milliseconds{ 50 });
    EXPECT_TRUE(terminated.Failed());
    EXPECT_NE(terminated.Message().find("Timed out"), std::string::npos);
}

TEST_F(ProcessTests, TerminateProcessWaitsForNonChildProcess)
{
    const auto current_process_id = process->GetCurrentProcessId();
    ASSERT_TRUE(current_process_id.Succeeded()) << current_process_id.Message();
    const auto pid_file = std::filesystem::temp_directory_path() /
        ("winux-grandchild-" + std::to_string(current_process_id.Value()) + ".pid");
    std::filesystem::remove(pid_file);

    const std::string script = "sleep 30 & echo $! > '" + pid_file.string() + "'; wait";
    const auto result = process->CreateProcess(
        std::wstring(L"sh -c \"") + std::wstring(script.begin(), script.end()) + L"\"");
    ASSERT_TRUE(result.Succeeded()) << result.Message();
    created_processes.push_back(result.Value());

    std::uint32_t descendant_process_id = 0;
    for (int attempt = 0; attempt < 100 && descendant_process_id == 0; ++attempt)
    {
        std::ifstream pid_stream(pid_file);
        pid_stream >> descendant_process_id;
        if (descendant_process_id == 0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds{ 1 });
        }
    }
    ASSERT_NE(descendant_process_id, 0u);

    const auto terminated = process->TerminateProcess(
        descendant_process_id,
        std::chrono::milliseconds{ 500 });
    EXPECT_TRUE(terminated.Succeeded()) << terminated.Message();
    std::filesystem::remove(pid_file);
}
#endif

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

TEST_F(ProcessTests, ExitedChildIsNotReportedAsRunning)
{
#ifdef _WIN32
    const auto result = process->CreateProcess(L"cmd.exe /c exit 0");
#else
    const auto result = process->CreateProcess(L"true");
#endif
    ASSERT_TRUE(result.Succeeded()) << result.Message();
    created_processes.push_back(result.Value());

    Winux::Result<bool> running = Winux::Result<bool>::Success(true);
    for (int attempt = 0; attempt < 100 && running.Value(); ++attempt)
    {
        running = process->IsRunning(result.Value());
        ASSERT_TRUE(running.Succeeded()) << running.Message();
        if (running.Value())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds{ 10 });
        }
    }
    EXPECT_FALSE(running.Value());
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
    EXPECT_TRUE(std::is_sorted(result.Value().begin(), result.Value().end()));
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

    Winux::CapabilitySet no_window_and_console;
    no_window_and_console.Add<Winux::Process::CreateNoWindow>();
    no_window_and_console.Add<Winux::Process::CreateNewConsole>();
    EXPECT_TRUE(process->CreateProcess(
        L"cmd.exe /c exit 0", no_window_and_console).Failed());

    Winux::CapabilitySet no_window_and_detached;
    no_window_and_detached.Add<Winux::Process::CreateNoWindow>();
    no_window_and_detached.Add<Winux::Process::Detached>();
    EXPECT_TRUE(process->CreateProcess(
        L"cmd.exe /c exit 0", no_window_and_detached).Failed());
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
