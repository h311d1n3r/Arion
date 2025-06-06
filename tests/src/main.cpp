#include <arion_test/common.hpp>

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new ArionGlobalEnv());
    return RUN_ALL_TESTS();
}

INSTANTIATE_TEST_SUITE_P(ArionTargetArchitectures, ArionMultiarchTest,
                         ::testing::Values("x86", "x86-64", "arm", "arm64"));
