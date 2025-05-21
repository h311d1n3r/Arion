#include <arion_test/common.hpp>

INSTANTIATE_TEST_SUITE_P(ArionTargetArchitectures, ArionMultiarchTest,
                         ::testing::Values("x86", "x86-64", "arm", "arm64"));
