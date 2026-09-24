#include <Winux/Winux.h>

#include <gtest/gtest.h>

#include <memory>

namespace {

class SystemTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        platform = Winux::Platform::create();
        ASSERT_NE(platform, nullptr);
        system = &platform->system();
    }

    std::unique_ptr<Winux::Contracts::IPlatform> platform;
    Winux::Contracts::ISystem* system = nullptr;
};

TEST_F(SystemTests, ReturnsCurrentUsername)
{
    ASSERT_NE(system, nullptr);

    const auto username = system->get_username();
    ASSERT_TRUE(username.succeeded()) << username.message();
    EXPECT_FALSE(username.value().empty());
}

}
