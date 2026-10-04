#include <Winux/Winux.h>
#include "TestPlatform.h"

#include <gtest/gtest.h>


namespace {

class SystemTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        Winux::Testing::InitializePlatformOnce();
        system = &Winux::Get<Winux::System>();
    }

    Winux::System* system = nullptr;
};

TEST_F(SystemTests, ReturnsCurrentUsername)
{
    ASSERT_NE(system, nullptr);

    const auto username = system->GetUsername();
    ASSERT_TRUE(username.Succeeded()) << username.Message();
    EXPECT_FALSE(username.Value().empty());
}

}
