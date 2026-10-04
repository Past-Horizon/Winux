#include <Winux/Winux.h>
#include "TestPlatform.h"

#include <gtest/gtest.h>

#include <any>
#include <functional>
#include <string>

namespace {

class TerminalTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        Winux::Testing::InitializePlatformOnce();
        terminal = &Winux::Get<Winux::Terminal>();
    }

    Winux::Terminal* terminal = nullptr;
};

TEST_F(TerminalTests, PlatformCreationProvidesTerminalApi)
{
    ASSERT_NE(terminal, nullptr);
}

TEST_F(TerminalTests, ExecuteCommandRejectsEmptyCommandLine)
{
    const auto result = terminal->ExecuteCommand(L"");
    EXPECT_TRUE(result.Failed());
    EXPECT_FALSE(result.Message().empty());
}

TEST_F(TerminalTests, ExecuteCommandCapturesOutput)
{
#ifdef _WIN32
    const auto result = terminal->ExecuteCommand(L"cmd.exe /c echo Winux");
#else
    const auto result = terminal->ExecuteCommand(L"printf Winux");
#endif

    ASSERT_TRUE(result.Succeeded()) << result.Message();
    EXPECT_NE(result.Value().find("Winux"), std::string::npos);
}

TEST_F(TerminalTests, CreateCommandExecutesAndNotifiesSubscribers)
{
    bool execute_called = false;
    bool can_execute_called = false;
    int changed_count = 0;

    auto handler = [&changed_count]() {
        ++changed_count;
    };

    const auto command = terminal->CreateCommand(
        [&execute_called](const std::any& parameter) -> Winux::Core::Result<void> {
            EXPECT_EQ(parameter.type(), typeid(std::wstring));
            EXPECT_EQ(std::any_cast<std::wstring>(parameter), L"payload");
            execute_called = true;
            return Winux::Core::Result<void>::Success();
        },
        [&can_execute_called](const std::any& parameter) -> bool {
            EXPECT_EQ(parameter.type(), typeid(std::wstring));
            can_execute_called = true;
            return std::any_cast<std::wstring>(parameter) == L"payload";
        });

    ASSERT_NE(command, nullptr);

    EXPECT_TRUE(command->CanExecute(std::wstring{L"payload"}));
    EXPECT_TRUE(can_execute_called);

    command->AddChanged(handler);
    command->RaiseChanged();
    EXPECT_EQ(changed_count, 1);

    command->RemoveChanged(handler);
    command->RaiseChanged();
    EXPECT_EQ(changed_count, 1);

    const auto execution = command->Execute(std::wstring{L"payload"});
    EXPECT_TRUE(execution.Succeeded()) << execution.Message();
    EXPECT_TRUE(execute_called);
}

} // namespace
