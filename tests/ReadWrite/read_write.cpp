#include <arion/arion.hpp>
#include <gtest/gtest.h>

class ArionTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        system("echo test");
    }
};

TEST_F(ArionTest, ReadWrite)
{
    EXPECT_STRNE("hello", "world");
    EXPECT_EQ(7 * 6, 42);
}
