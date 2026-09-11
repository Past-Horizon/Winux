#include <Winux/Winux.h>

#include <gtest/gtest.h>

#ifdef _WIN32

#include <memory>
#include <string>

namespace {

class MutexTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        platform = Winux::Platform::create();
        ASSERT_NE(platform, nullptr);
    }

    std::unique_ptr<Winux::Contracts::IPlatform> platform;
};

std::unique_ptr<Winux::Contracts::IMutex> CreateMutex(
    Winux::Contracts::IPlatform& platform,
    const std::wstring& name)
{
    auto result = platform.create_mutex(name);
    EXPECT_TRUE(result.succeeded()) << result.message();
    if (result.failed())
    {
        return nullptr;
    }

    return std::move(result).value();
}

TEST_F(MutexTests, EmptyNamesAreRejected)
{
    const auto result = platform->create_mutex(L"");
    EXPECT_TRUE(result.failed());
    EXPECT_FALSE(result.message().empty());
}

TEST_F(MutexTests, SameNameAllowsOnlyOneOwner)
{
    auto first = CreateMutex(*platform, L"Winux.MutexTests.Contention");
    auto second = CreateMutex(*platform, L"Winux.MutexTests.Contention");
    ASSERT_NE(first, nullptr);
    ASSERT_NE(second, nullptr);

    const auto first_acquired = first->try_acquire();
    ASSERT_TRUE(first_acquired.succeeded()) << first_acquired.message();
    EXPECT_TRUE(first_acquired.value());
    EXPECT_TRUE(first->owns_lock());

    const auto second_acquired = second->try_acquire();
    ASSERT_TRUE(second_acquired.succeeded()) << second_acquired.message();
    EXPECT_FALSE(second_acquired.value());
    EXPECT_FALSE(second->owns_lock());

    ASSERT_TRUE(first->release().succeeded());
    const auto retry = second->try_acquire();
    ASSERT_TRUE(retry.succeeded()) << retry.message();
    EXPECT_TRUE(retry.value());
}

TEST_F(MutexTests, DifferentNamesDoNotContend)
{
    auto first = CreateMutex(*platform, L"Winux.MutexTests.First");
    auto second = CreateMutex(*platform, L"Winux.MutexTests.Second");
    ASSERT_NE(first, nullptr);
    ASSERT_NE(second, nullptr);

    ASSERT_TRUE(first->try_acquire().value());
    const auto second_acquired = second->try_acquire();
    ASSERT_TRUE(second_acquired.succeeded()) << second_acquired.message();
    EXPECT_TRUE(second_acquired.value());
}

TEST_F(MutexTests, ReleaseIsIdempotentAndDestructionReleases)
{
    {
        auto mutex = CreateMutex(*platform, L"Winux.MutexTests.Lifetime");
        ASSERT_NE(mutex, nullptr);
        ASSERT_TRUE(mutex->try_acquire().value());
        ASSERT_TRUE(mutex->release().succeeded());
        EXPECT_FALSE(mutex->owns_lock());
        ASSERT_TRUE(mutex->release().succeeded());
    }

    auto replacement = CreateMutex(*platform, L"Winux.MutexTests.Lifetime");
    ASSERT_NE(replacement, nullptr);
    const auto acquired = replacement->try_acquire();
    ASSERT_TRUE(acquired.succeeded()) << acquired.message();
    EXPECT_TRUE(acquired.value());
}

TEST_F(MutexTests, UnusualNamesRemainSafe)
{
    const std::wstring name = L"Winux/Mutex\\Tests:*?\"<>|\u0000";
    auto mutex = CreateMutex(*platform, name);
    ASSERT_NE(mutex, nullptr);

    const auto acquired = mutex->try_acquire();
    ASSERT_TRUE(acquired.succeeded()) << acquired.message();
    EXPECT_TRUE(acquired.value());
}

}

#endif
