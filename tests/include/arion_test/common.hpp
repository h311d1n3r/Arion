#ifndef ARION_TEST_COMMON_HPP
#define ARION_TEST_COMMON_HPP

#include <gtest/gtest.h>

class ArionTest : public ::testing::Test {
protected:
    std::string targets_path;

    void SetUp() override {
        this->targets_path = std::string(ARION_TEST_TARGETS_PATH);
    }

};

#endif //ARION_TEST_COMMON_HPP
