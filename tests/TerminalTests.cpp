#include <Winux/Winux.h>

#include <gtest/gtest.h>

#include <any>
#include <functional>
#include <memory>
#include <string>

namespace {

class TerminalTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        platform = Winux::Platform::create();
        ASSERT_NE(platform, nullptr);
        terminal = &platform->terminal();
    }

    std::unique_ptr<Winux::Contracts::IPlatform> platform;
    Winux::Contracts::ITerminal* terminal = nullptr;
};

TEST_F(TerminalTests, PlatformCreationProvidesTerminalApi)
{
    ASSERT_NE(platform, nullptr);
    ASSERT_NE(terminal, nullptr);
}

TEST_F(TerminalTests, ExecuteCommandRejectsEmptyCommandLine)
{
    const auto result = terminal->execute_command(L"");
    EXPECT_TRUE(result.failed());
    EXPECT_FALSE(result.message().empty());
}

TEST_F(TerminalTests, ExecuteCommandCapturesOutput)
{
#ifdef _WIN32
    const auto result = terminal->execute_command(L"cmd.exe /c echo Winux");
#else
    const auto result = terminal->execute_command(L"printf Winux");
#endif

    ASSERT_TRUE(result.succeeded()) << result.message();
    EXPECT_NE(result.value().find("Winux"), std::string::npos);
}

TEST_F(TerminalTests, CreateCommandExecutesAndNotifiesSubscribers)
{
    bool execute_called = false;
    bool can_execute_called = false;
    int changed_count = 0;

    auto handler = [&changed_count]() {
        ++changed_count;
    };

    const auto command = terminal->create_command(
        [&execute_called](const std::any& parameter) -> Winux::Core::Result<void> {
            EXPECT_EQ(parameter.type(), typeid(std::wstring));
            EXPECT_EQ(std::any_cast<std::wstring>(parameter), L"payload");
            execute_called = true;
            return Winux::Core::Result<void>::success();
        },
        [&can_execute_called](const std::any& parameter) -> bool {
            EXPECT_EQ(parameter.type(), typeid(std::wstring));
            can_execute_called = true;
            return std::any_cast<std::wstring>(parameter) == L"payload";
        });

    ASSERT_NE(command, nullptr);

    EXPECT_TRUE(command->can_execute(std::wstring{L"payload"}));
    EXPECT_TRUE(can_execute_called);

    command->add_changed(handler);
    command->raise_changed();
    EXPECT_EQ(changed_count, 1);

    command->remove_changed(handler);
    command->raise_changed();
    EXPECT_EQ(changed_count, 1);

    const auto execution = command->execute(std::wstring{L"payload"});
    EXPECT_TRUE(execution.succeeded()) << execution.message();
    EXPECT_TRUE(execute_called);
}

} // namespace
