#include <Winux/Winux.h>
#include "TestPlatform.h"

#include <gtest/gtest.h>

#include <array>
#include <filesystem>
#include <string>

namespace {

class FileSystemTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        Winux::Testing::InitializePlatformOnce();
        file_system = &Winux::Get<Winux::FileSystem>();
        environment = &Winux::Get<Winux::Environment>();
    }

    Winux::FileSystem* file_system = nullptr;
    Winux::Environment* environment = nullptr;
};

TEST_F(FileSystemTests, PlatformCreationProvidesFileSystemApi)
{
    ASSERT_NE(file_system, nullptr);
}

TEST_F(FileSystemTests, KnownFoldersReturnPaths)
{
    const std::array paths{
        file_system->Home(),
        file_system->Desktop(),
        file_system->AppData(),
        file_system->Temp()};

    for (const auto& result : paths)
    {
        ASSERT_TRUE(result.Succeeded()) << result.Message();
        EXPECT_FALSE(result.Value().empty());
    }
}

TEST_F(FileSystemTests, AppDataScopesAreComposable)
{
    const auto local = file_system->AppData();
    const auto explicit_local = file_system->AppData(Winux::AppDataScope::Local);
    const auto local_low = file_system->AppData(Winux::AppDataScope::LocalLow);
    const auto roaming = file_system->AppData(Winux::AppDataScope::Roaming);

    ASSERT_TRUE(local.Succeeded()) << local.Message();
    ASSERT_TRUE(explicit_local.Succeeded()) << explicit_local.Message();
    ASSERT_TRUE(local_low.Succeeded()) << local_low.Message();
    ASSERT_TRUE(roaming.Succeeded()) << roaming.Message();
    EXPECT_EQ(local.Value(), explicit_local.Value());
    EXPECT_FALSE((local.Value() / "Winux").empty());
    EXPECT_NE(local.Value(), local_low.Value());
    EXPECT_NE(local.Value(), roaming.Value());
}

TEST_F(FileSystemTests, EnvironmentVariableRoundTrips)
{
    constexpr auto name = L"WINUX_TEST_ENVIRONMENT_VARIABLE";
    ASSERT_NE(environment, nullptr);
    ASSERT_TRUE(environment->UnsetEnv(name).Succeeded());

    const auto missing = environment->GetEnv(name);
    EXPECT_TRUE(missing.Failed());

    ASSERT_TRUE(environment->SetEnv(name, L"winux-value").Succeeded());
    const auto value = environment->GetEnv(name);
    ASSERT_TRUE(value.Succeeded()) << value.Message();
    EXPECT_EQ(value.Value(), L"winux-value");

    ASSERT_TRUE(environment->UnsetEnv(name).Succeeded());
    EXPECT_TRUE(environment->GetEnv(name).Failed());
}

TEST_F(FileSystemTests, WriteAndReadFileSucceeds)
{
    const auto temp = file_system->Temp();
    ASSERT_TRUE(temp.Succeeded()) << temp.Message();

    const std::filesystem::path file = temp.Value() / "winux-filesystem-test.txt";
    std::error_code cleanup_error;
    std::filesystem::remove(file, cleanup_error);

    const auto written = file_system->WriteFile(file, "Winux file test");
    ASSERT_TRUE(written.Succeeded()) << written.Message();

    const auto read = file_system->ReadFile(file);
    ASSERT_TRUE(read.Succeeded()) << read.Message();
    EXPECT_EQ(read.Value(), "Winux file test");

    std::filesystem::remove(file, cleanup_error);
}

TEST_F(FileSystemTests, CommonFilesystemOperationsReturnResults)
{
    const auto temp = file_system->Temp();
    ASSERT_TRUE(temp.Succeeded()) << temp.Message();

    const auto directory = temp.Value() / "winux-filesystem-operations" / "nested";
    const auto created = file_system->CreateDirectories(directory);
    ASSERT_TRUE(created.Succeeded()) << created.Message();
    EXPECT_TRUE(created.Value());

    const auto already_created = file_system->CreateDirectories(directory);
    ASSERT_TRUE(already_created.Succeeded()) << already_created.Message();
    EXPECT_FALSE(already_created.Value());

    const auto is_directory = file_system->IsDirectory(directory);
    ASSERT_TRUE(is_directory.Succeeded()) << is_directory.Message();
    EXPECT_TRUE(is_directory.Value());

    const auto exists = file_system->Exists(directory);
    ASSERT_TRUE(exists.Succeeded()) << exists.Message();
    EXPECT_TRUE(exists.Value());

    const auto file = directory / "size.txt";
    ASSERT_TRUE(file_system->WriteFile(file, "size").Succeeded());
    const auto file_exists = file_system->Exists(file);
    ASSERT_TRUE(file_exists.Succeeded()) << file_exists.Message();
    EXPECT_TRUE(file_exists.Value());

    const auto file_size = file_system->FileSize(file);
    ASSERT_TRUE(file_size.Succeeded()) << file_size.Message();
    EXPECT_EQ(file_size.Value(), 4u);

    const auto removed_file = file_system->Remove(file);
    ASSERT_TRUE(removed_file.Succeeded()) << removed_file.Message();
    EXPECT_TRUE(removed_file.Value());

    const auto removed_directory = file_system->Remove(directory);
    ASSERT_TRUE(removed_directory.Succeeded()) << removed_directory.Message();
    EXPECT_TRUE(removed_directory.Value());

    const auto removed_missing = file_system->Remove(file);
    ASSERT_TRUE(removed_missing.Succeeded()) << removed_missing.Message();
    EXPECT_FALSE(removed_missing.Value());

    const auto missing = file_system->FileSize(file);
    EXPECT_TRUE(missing.Failed());
    EXPECT_FALSE(missing.Message().empty());

    std::error_code cleanup_error;
    std::filesystem::remove_all(directory.parent_path(), cleanup_error);
}

TEST_F(FileSystemTests, MoveFileSucceeds)
{
    const auto temp = file_system->Temp();
    ASSERT_TRUE(temp.Succeeded()) << temp.Message();

    const std::filesystem::path source = temp.Value() / "winux-filesystem-move-source.txt";
    const std::filesystem::path destination = temp.Value() / "winux-filesystem-move-destination.txt";
    std::error_code cleanup_error;
    std::filesystem::remove(source, cleanup_error);
    std::filesystem::remove(destination, cleanup_error);

    ASSERT_TRUE(file_system->WriteFile(source, "Winux move test").Succeeded());
    const auto moved = file_system->MoveFile(source, destination);
    ASSERT_TRUE(moved.Succeeded()) << moved.Message();
    EXPECT_FALSE(std::filesystem::exists(source));
    EXPECT_TRUE(std::filesystem::exists(destination));

    const auto contents = file_system->ReadFile(destination);
    ASSERT_TRUE(contents.Succeeded()) << contents.Message();
    EXPECT_EQ(contents.Value(), "Winux move test");

    std::filesystem::remove(destination, cleanup_error);
}

TEST_F(FileSystemTests, MoveFileReplacesExistingDestination)
{
    const auto temp = file_system->Temp();
    ASSERT_TRUE(temp.Succeeded()) << temp.Message();

    const std::filesystem::path source = temp.Value() / "winux-filesystem-replace-source.txt";
    const std::filesystem::path destination = temp.Value() / "winux-filesystem-replace-destination.txt";
    std::error_code cleanup_error;
    std::filesystem::remove(source, cleanup_error);
    std::filesystem::remove(destination, cleanup_error);

    ASSERT_TRUE(file_system->WriteFile(source, "new contents").Succeeded());
    ASSERT_TRUE(file_system->WriteFile(destination, "old contents").Succeeded());

    const auto moved = file_system->MoveFile(source, destination);
    ASSERT_TRUE(moved.Succeeded()) << moved.Message();
    EXPECT_FALSE(std::filesystem::exists(source));

    const auto contents = file_system->ReadFile(destination);
    ASSERT_TRUE(contents.Succeeded()) << contents.Message();
    EXPECT_EQ(contents.Value(), "new contents");

    std::filesystem::remove(destination, cleanup_error);
}

TEST_F(FileSystemTests, MoveFileFailuresReturnFailure)
{
    const auto temp = file_system->Temp();
    ASSERT_TRUE(temp.Succeeded()) << temp.Message();

    const std::filesystem::path missing_source = temp.Value() / "winux-missing-move-source.txt";
    const std::filesystem::path destination = temp.Value() / "winux-move-destination.txt";
    std::error_code cleanup_error;
    std::filesystem::remove(missing_source, cleanup_error);
    std::filesystem::remove(destination, cleanup_error);

    const auto missing = file_system->MoveFile(missing_source, destination);
    EXPECT_TRUE(missing.Failed());
    EXPECT_FALSE(missing.Message().empty());

    const std::filesystem::path source = temp.Value() / "winux-move-source.txt";
    const std::filesystem::path invalid_destination =
        temp.Value() / "winux-missing-move-directory" / "destination.txt";
    std::filesystem::remove(source, cleanup_error);
    ASSERT_TRUE(file_system->WriteFile(source, "data").Succeeded());

    const auto invalid = file_system->MoveFile(source, invalid_destination);
    EXPECT_TRUE(invalid.Failed());
    EXPECT_FALSE(invalid.Message().empty());

    std::filesystem::remove(source, cleanup_error);
}

TEST_F(FileSystemTests, ReadAndWriteFailuresReturnFailure)
{
    const auto temp = file_system->Temp();
    ASSERT_TRUE(temp.Succeeded()) << temp.Message();

    const std::filesystem::path missing_file = temp.Value() / "winux-file-does-not-exist.txt";
    std::error_code cleanup_error;
    std::filesystem::remove(missing_file, cleanup_error);

    const auto read = file_system->ReadFile(missing_file);
    EXPECT_TRUE(read.Failed());
    EXPECT_FALSE(read.Message().empty());

    const std::filesystem::path invalid_file =
        temp.Value() / "winux-missing-directory" / "file.txt";
    const auto written = file_system->WriteFile(invalid_file, "data");
    EXPECT_TRUE(written.Failed());
    EXPECT_FALSE(written.Message().empty());
}

}
