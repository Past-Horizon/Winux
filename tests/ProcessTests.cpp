#include <Winux/Winux.h>

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
        platform = Winux::Platform::create();
        ASSERT_NE(platform, nullptr);
        process = &platform->process();
    }

    void TearDown() override
    {
        for (const std::uint32_t process_id : created_processes)
        {
            if (process->is_running(process_id).succeeded())
            {
                process->terminate_process(process_id);
            }
        }
    }

    std::uint32_t create_test_process()
    {
#ifdef _WIN32
        const auto result = process->create_process(
            L"cmd.exe /c \"ping 127.0.0.1 -n 30 > nul\"")
            .start();
#else
        const auto result = process->create_process(L"sleep 30").start();
#endif
        EXPECT_TRUE(result.succeeded()) << result.message();
        if (result.failed())
        {
            return 0;
        }

        created_processes.push_back(result.value());
        return result.value();
    }

    std::unique_ptr<Winux::Contracts::IPlatform> platform;
    Winux::Contracts::IProcess* process = nullptr;
    std::vector<std::uint32_t> created_processes;
};

TEST_F(ProcessTests, PlatformCreationProvidesProcessApi)
{
    ASSERT_NE(platform, nullptr);
    ASSERT_NE(process, nullptr);
}

TEST_F(ProcessTests, ProcessOptionsMatchPlatformSupport)
{
    const auto supported = platform->supported_features();
#ifdef _WIN32
    EXPECT_TRUE(supported.contains(
        Winux::Contracts::IProcess::ProcessOption::CreateNoWindow));
    EXPECT_TRUE(supported.contains(
        Winux::Contracts::IProcess::ProcessOption::CreateNewConsole));
#else
    EXPECT_FALSE(supported.contains(
        Winux::Contracts::IProcess::ProcessOption::CreateNoWindow));
    EXPECT_FALSE(supported.contains(
        Winux::Contracts::IProcess::ProcessOption::CreateNewConsole));
#endif
}

TEST_F(ProcessTests, CreateProcessReturnsRunningProcess)
{
    const std::uint32_t process_id = create_test_process();
    ASSERT_NE(process_id, 0u);

    const auto running = process->is_running(process_id);
    ASSERT_TRUE(running.succeeded()) << running.message();
    EXPECT_TRUE(running.value());
}

TEST_F(ProcessTests, FindLocationReturnsExecutablePath)
{
    const std::uint32_t process_id = create_test_process();
    ASSERT_NE(process_id, 0u);

    const auto location = process->find_location(process_id);
    ASSERT_TRUE(location.succeeded()) << location.message();
    EXPECT_TRUE(std::filesystem::exists(location.value()));
    EXPECT_FALSE(location.value().empty());
}

TEST_F(ProcessTests, FindLocationRejectsInvalidProcess)
{
    const auto location = process->find_location(0);
    EXPECT_TRUE(location.failed());
}

TEST_F(ProcessTests, FindProcessFindsCreatedProcess)
{
    const std::uint32_t process_id = create_test_process();
    ASSERT_NE(process_id, 0u);

#ifdef _WIN32
    const auto result = process->find_process(L"cmd.exe");
#else
    const auto result = process->find_process(L"sleep");
#endif
    ASSERT_TRUE(result.succeeded()) << result.message();
    ASSERT_TRUE(result.value().has_value());
}

TEST_F(ProcessTests, FindProcessReportsMissingProcess)
{
    const auto result = process->find_process(L"winux-process-that-does-not-exist");
    ASSERT_TRUE(result.succeeded()) << result.message();
    EXPECT_FALSE(result.value().has_value());
}

TEST_F(ProcessTests, FindProcessesReturnsCreatedProcess)
{
    const std::uint32_t process_id = create_test_process();
    ASSERT_NE(process_id, 0u);

#ifdef _WIN32
    const auto result = process->find_processes(L"cmd.exe");
#else
    const auto result = process->find_processes(L"sleep");
#endif
    ASSERT_TRUE(result.succeeded()) << result.message();
    EXPECT_FALSE(result.value().empty());
}

TEST_F(ProcessTests, IsRunningChangesAfterTermination)
{
    const std::uint32_t process_id = create_test_process();
    ASSERT_NE(process_id, 0u);

    const auto before = process->is_running(process_id);
    ASSERT_TRUE(before.succeeded()) << before.message();
    ASSERT_TRUE(before.value());

    const auto terminated = process->terminate_process(process_id);
    ASSERT_TRUE(terminated.succeeded()) << terminated.message();

    const auto after = process->is_running(process_id);
    ASSERT_TRUE(after.succeeded()) << after.message();
    EXPECT_FALSE(after.value());
}

TEST_F(ProcessTests, TerminateRejectsInvalidProcess)
{
    const auto terminated = process->terminate_process(0);
    EXPECT_TRUE(terminated.failed());
}

#ifdef _WIN32
TEST_F(ProcessTests, NoWindowOptionStartsProcess)
{
    const auto result = process->create_process(
        L"cmd.exe /c \"ping 127.0.0.1 -n 30 > nul\"")
        .no_window()
        .start();
    ASSERT_TRUE(result.succeeded()) << result.message();
    created_processes.push_back(result.value());

    const auto running = process->is_running(result.value());
    ASSERT_TRUE(running.succeeded()) << running.message();
    EXPECT_TRUE(running.value());
}

TEST_F(ProcessTests, NewConsoleOptionStartsProcess)
{
    const auto result = process->create_process(
        L"cmd.exe /c \"ping 127.0.0.1 -n 30 > nul\"")
        .new_console()
        .start();
    ASSERT_TRUE(result.succeeded()) << result.message();
    created_processes.push_back(result.value());

    const auto running = process->is_running(result.value());
    ASSERT_TRUE(running.succeeded()) << running.message();
    EXPECT_TRUE(running.value());
}
#else
TEST_F(ProcessTests, UnsupportedWindowsOptionsFailBeforeStarting)
{
    const auto no_window = process->create_process(L"sleep 30").no_window().start();
    EXPECT_TRUE(no_window.failed());

    const auto new_console = process->create_process(L"sleep 30").new_console().start();
    EXPECT_TRUE(new_console.failed());
}
#endif

} // namespace
