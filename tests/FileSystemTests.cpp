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
        environment = &platform->environment();
    }

    std::unique_ptr<Winux::Contracts::IPlatform> platform;
    Winux::Contracts::IFileSystem* file_system = nullptr;
    Winux::Contracts::IEnvironment* environment = nullptr;
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
        file_system->app_data().start(),
        file_system->temp()};

    for (const auto& result : paths)
    {
        ASSERT_TRUE(result.succeeded()) << result.message();
        EXPECT_FALSE(result.value().empty());
    }
}

TEST_F(FileSystemTests, AppDataScopesAreComposable)
{
    const auto local = file_system->app_data().start();
    const auto explicit_local = file_system->app_data().local().start();
    const auto local_low = file_system->app_data().local_low().start();
    const auto roaming = file_system->app_data().roaming().start();

    ASSERT_TRUE(local.succeeded()) << local.message();
    ASSERT_TRUE(explicit_local.succeeded()) << explicit_local.message();
    ASSERT_TRUE(local_low.succeeded()) << local_low.message();
    ASSERT_TRUE(roaming.succeeded()) << roaming.message();
    EXPECT_EQ(local.value(), explicit_local.value());
    EXPECT_FALSE((local.value() / "Winux").empty());
    EXPECT_NE(local.value(), local_low.value());
    EXPECT_NE(local.value(), roaming.value());
}

TEST_F(FileSystemTests, EnvironmentVariableRoundTrips)
{
    constexpr auto name = L"WINUX_TEST_ENVIRONMENT_VARIABLE";
    ASSERT_NE(environment, nullptr);
    ASSERT_TRUE(environment->unset_env(name).succeeded());

    const auto missing = environment->get_env(name);
    EXPECT_TRUE(missing.failed());

    ASSERT_TRUE(environment->set_env(name, L"winux-value").succeeded());
    const auto value = environment->get_env(name);
    ASSERT_TRUE(value.succeeded()) << value.message();
    EXPECT_EQ(value.value(), L"winux-value");

    ASSERT_TRUE(environment->unset_env(name).succeeded());
    EXPECT_TRUE(environment->get_env(name).failed());
}

TEST_F(FileSystemTests, WriteAndReadFileSucceeds)
{
    const auto temp = file_system->temp();
    ASSERT_TRUE(temp.succeeded()) << temp.message();

    const std::filesystem::path file = temp.value() / "winux-filesystem-test.txt";
    std::error_code cleanup_error;
    std::filesystem::remove(file, cleanup_error);

    const auto written = file_system->write_file(file, "Winux file test");
    ASSERT_TRUE(written.succeeded()) << written.message();

    const auto read = file_system->read_file(file);
    ASSERT_TRUE(read.succeeded()) << read.message();
    EXPECT_EQ(read.value(), "Winux file test");

    std::filesystem::remove(file, cleanup_error);
}

TEST_F(FileSystemTests, MoveFileSucceeds)
{
    const auto temp = file_system->temp();
    ASSERT_TRUE(temp.succeeded()) << temp.message();

    const std::filesystem::path source = temp.value() / "winux-filesystem-move-source.txt";
    const std::filesystem::path destination = temp.value() / "winux-filesystem-move-destination.txt";
    std::error_code cleanup_error;
    std::filesystem::remove(source, cleanup_error);
    std::filesystem::remove(destination, cleanup_error);

    ASSERT_TRUE(file_system->write_file(source, "Winux move test").succeeded());
    const auto moved = file_system->move_file(source, destination);
    ASSERT_TRUE(moved.succeeded()) << moved.message();
    EXPECT_FALSE(std::filesystem::exists(source));
    EXPECT_TRUE(std::filesystem::exists(destination));

    const auto contents = file_system->read_file(destination);
    ASSERT_TRUE(contents.succeeded()) << contents.message();
    EXPECT_EQ(contents.value(), "Winux move test");

    std::filesystem::remove(destination, cleanup_error);
}

TEST_F(FileSystemTests, MoveFileReplacesExistingDestination)
{
    const auto temp = file_system->temp();
    ASSERT_TRUE(temp.succeeded()) << temp.message();

    const std::filesystem::path source = temp.value() / "winux-filesystem-replace-source.txt";
    const std::filesystem::path destination = temp.value() / "winux-filesystem-replace-destination.txt";
    std::error_code cleanup_error;
    std::filesystem::remove(source, cleanup_error);
    std::filesystem::remove(destination, cleanup_error);

    ASSERT_TRUE(file_system->write_file(source, "new contents").succeeded());
    ASSERT_TRUE(file_system->write_file(destination, "old contents").succeeded());

    const auto moved = file_system->move_file(source, destination);
    ASSERT_TRUE(moved.succeeded()) << moved.message();
    EXPECT_FALSE(std::filesystem::exists(source));

    const auto contents = file_system->read_file(destination);
    ASSERT_TRUE(contents.succeeded()) << contents.message();
    EXPECT_EQ(contents.value(), "new contents");

    std::filesystem::remove(destination, cleanup_error);
}

TEST_F(FileSystemTests, MoveFileFailuresReturnFailure)
{
    const auto temp = file_system->temp();
    ASSERT_TRUE(temp.succeeded()) << temp.message();

    const std::filesystem::path missing_source = temp.value() / "winux-missing-move-source.txt";
    const std::filesystem::path destination = temp.value() / "winux-move-destination.txt";
    std::error_code cleanup_error;
    std::filesystem::remove(missing_source, cleanup_error);
    std::filesystem::remove(destination, cleanup_error);

    const auto missing = file_system->move_file(missing_source, destination);
    EXPECT_TRUE(missing.failed());
    EXPECT_FALSE(missing.message().empty());

    const std::filesystem::path source = temp.value() / "winux-move-source.txt";
    const std::filesystem::path invalid_destination =
        temp.value() / "winux-missing-move-directory" / "destination.txt";
    std::filesystem::remove(source, cleanup_error);
    ASSERT_TRUE(file_system->write_file(source, "data").succeeded());

    const auto invalid = file_system->move_file(source, invalid_destination);
    EXPECT_TRUE(invalid.failed());
    EXPECT_FALSE(invalid.message().empty());

    std::filesystem::remove(source, cleanup_error);
}

TEST_F(FileSystemTests, ReadAndWriteFailuresReturnFailure)
{
    const auto temp = file_system->temp();
    ASSERT_TRUE(temp.succeeded()) << temp.message();

    const std::filesystem::path missing_file = temp.value() / "winux-file-does-not-exist.txt";
    std::error_code cleanup_error;
    std::filesystem::remove(missing_file, cleanup_error);

    const auto read = file_system->read_file(missing_file);
    EXPECT_TRUE(read.failed());
    EXPECT_FALSE(read.message().empty());

    const std::filesystem::path invalid_file =
        temp.value() / "winux-missing-directory" / "file.txt";
    const auto written = file_system->write_file(invalid_file, "data");
    EXPECT_TRUE(written.failed());
    EXPECT_FALSE(written.message().empty());
}

}
