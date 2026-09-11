#include <Winux/Winux.h>

#include <gtest/gtest.h>

#include <array>
#include <filesystem>
#include <memory>
#include <string>

namespace {

class FileSystemTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        platform = Winux::Platform::create();
        ASSERT_NE(platform, nullptr);
        file_system = &platform->file_system();
    }

    std::unique_ptr<Winux::Contracts::IPlatform> platform;
    Winux::Contracts::IFileSystem* file_system = nullptr;
};

TEST_F(FileSystemTests, PlatformCreationProvidesFileSystemApi)
{
    ASSERT_NE(file_system, nullptr);
}

TEST_F(FileSystemTests, KnownFoldersReturnPaths)
{
    const std::array paths{
        file_system->home(),
        file_system->desktop(),
        file_system->app_data(),
        file_system->temp()};

    for (const auto& result : paths)
    {
        ASSERT_TRUE(result.succeeded()) << result.message();
        EXPECT_FALSE(result.value().empty());
    }
}

TEST_F(FileSystemTests, EnvironmentVariableRoundTrips)
{
    constexpr auto name = L"WINUX_TEST_ENVIRONMENT_VARIABLE";
    ASSERT_TRUE(file_system->unset_env(name).succeeded());

    const auto missing = file_system->get_env(name);
    EXPECT_TRUE(missing.failed());

    ASSERT_TRUE(file_system->set_env(name, L"winux-value").succeeded());
    const auto value = file_system->get_env(name);
    ASSERT_TRUE(value.succeeded()) << value.message();
    EXPECT_EQ(value.value(), L"winux-value");

    ASSERT_TRUE(file_system->unset_env(name).succeeded());
    EXPECT_TRUE(file_system->get_env(name).failed());
}

}
