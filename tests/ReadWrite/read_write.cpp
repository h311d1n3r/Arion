#include <gtest/gtest.h>

TEST(ArionTest, ReadWrite)
{
    EXPECT_STRNE("hello", "world");
    EXPECT_EQ(7 * 6, 42);
}
