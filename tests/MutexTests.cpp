#include <Winux/Winux.h>
#include "TestPlatform.h"

#include <gtest/gtest.h>

#include <memory>
#include <string>

namespace {

class MutexTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        Winux::Testing::InitializePlatformOnce();
        platform = &Winux::Get<Winux::PlatformContext>();
    }

    Winux::PlatformContext* platform = nullptr;
};

std::unique_ptr<Winux::Mutex> CreateMutex(
    Winux::PlatformContext& platform,
    const std::wstring& name)
{
    auto result = platform.CreateMutex(name);
    EXPECT_TRUE(result.Succeeded()) << result.Message();
    if (result.Failed())
    {
        return nullptr;
    }

    return std::move(result).Value();
}

TEST_F(MutexTests, EmptyNamesAreRejected)
{
    const auto result = platform->CreateMutex(L"");
    EXPECT_TRUE(result.Failed());
    EXPECT_FALSE(result.Message().empty());
}

TEST_F(MutexTests, SameNameAllowsOnlyOneOwner)
{
    auto first = CreateMutex(*platform, L"Winux.MutexTests.Contention");
    auto second = CreateMutex(*platform, L"Winux.MutexTests.Contention");
    ASSERT_NE(first, nullptr);
    ASSERT_NE(second, nullptr);

    const auto first_acquired = first->TryAcquire();
    ASSERT_TRUE(first_acquired.Succeeded()) << first_acquired.Message();
    EXPECT_TRUE(first_acquired.Value());
    EXPECT_TRUE(first->OwnsLock());

    const auto second_acquired = second->TryAcquire();
    ASSERT_TRUE(second_acquired.Succeeded()) << second_acquired.Message();
    EXPECT_FALSE(second_acquired.Value());
    EXPECT_FALSE(second->OwnsLock());

    ASSERT_TRUE(first->Release().Succeeded());
    const auto retry = second->TryAcquire();
    ASSERT_TRUE(retry.Succeeded()) << retry.Message();
    EXPECT_TRUE(retry.Value());
}

TEST_F(MutexTests, DifferentNamesDoNotContend)
{
    auto first = CreateMutex(*platform, L"Winux.MutexTests.First");
    auto second = CreateMutex(*platform, L"Winux.MutexTests.Second");
    ASSERT_NE(first, nullptr);
    ASSERT_NE(second, nullptr);

    ASSERT_TRUE(first->TryAcquire().Value());
    const auto second_acquired = second->TryAcquire();
    ASSERT_TRUE(second_acquired.Succeeded()) << second_acquired.Message();
    EXPECT_TRUE(second_acquired.Value());
}

TEST_F(MutexTests, ReleaseIsIdempotentAndDestructionReleases)
{
    {
        auto mutex = CreateMutex(*platform, L"Winux.MutexTests.Lifetime");
        ASSERT_NE(mutex, nullptr);
        ASSERT_TRUE(mutex->TryAcquire().Value());
        ASSERT_TRUE(mutex->Release().Succeeded());
        EXPECT_FALSE(mutex->OwnsLock());
        ASSERT_TRUE(mutex->Release().Succeeded());
    }

    auto replacement = CreateMutex(*platform, L"Winux.MutexTests.Lifetime");
    ASSERT_NE(replacement, nullptr);
    const auto acquired = replacement->TryAcquire();
    ASSERT_TRUE(acquired.Succeeded()) << acquired.Message();
    EXPECT_TRUE(acquired.Value());
}

TEST_F(MutexTests, UnusualNamesRemainSafe)
{
    const std::wstring name = L"Winux/Mutex\\Tests:*?\"<>|\u0000";
    auto mutex = CreateMutex(*platform, name);
    ASSERT_NE(mutex, nullptr);

    const auto acquired = mutex->TryAcquire();
    ASSERT_TRUE(acquired.Succeeded()) << acquired.Message();
    EXPECT_TRUE(acquired.Value());
}

}

