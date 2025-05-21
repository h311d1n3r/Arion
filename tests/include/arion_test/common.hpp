#ifndef ARION_TEST_COMMON_HPP
#define ARION_TEST_COMMON_HPP

#include <algorithm>
#include <filesystem>
#include <gtest/gtest.h>
#include <string>
#include <vector>

class ArionTest : public ::testing::Test
{
  protected:
    std::string arion_root_path;

    void SetUp() override
    {
        this->arion_root_path = std::string(ARION_ROOT_PATH);
    }
};

class ArionMultiarchTest : public ArionTest, public ::testing::WithParamInterface<std::string>
{
  protected:
    static inline std::vector<std::string> targets;
    static inline std::vector<std::string> archs;
    std::string arch;

    void SetUp() override
    {
        ArionTest::SetUp();
        this->arch = GetParam();
    }

    static void SetUpTestSuite()
    {
        ArionMultiarchTest::targets.clear();
        ArionMultiarchTest::archs.clear();
        std::string arion_root_path = std::string(ARION_ROOT_PATH);
        std::string targets_dir = arion_root_path + "/tests/targets";
        for (const auto &target_entry : std::filesystem::directory_iterator(targets_dir))
        {
            if (!target_entry.is_directory())
                continue;
            std::string target_name = target_entry.path().filename().string();
            ArionMultiarchTest::targets.push_back(target_name);
            for (const auto &arch_entry : std::filesystem::directory_iterator(target_entry))
            {
                auto arch_entry_path = arch_entry.path();
                std::string arch_entry_name = arch_entry_path.filename().string();
                std::string build_prefix = "build_";
                if (!arch_entry.is_directory() || arch_entry_name.rfind(build_prefix, 0))
                    continue;
                std::string arch = arch_entry_name.substr(build_prefix.length());
                if (std::find(ArionMultiarchTest::archs.begin(), ArionMultiarchTest::archs.end(), arch) ==
                    ArionMultiarchTest::archs.end())
                    ArionMultiarchTest::archs.push_back(arch);
                std::string rootfs_path = arion_root_path + "/rootfs/" + arch + "/rootfs";
                std::string target_rootfs_path = rootfs_path + "/root/" + target_name;
                if (std::filesystem::exists(target_rootfs_path))
                    std::filesystem::remove_all(target_rootfs_path);
                std::filesystem::create_directory(target_rootfs_path);
                std::string target_src_path = arch_entry_path.string() + "/" + target_name;
                std::string target_dest_path = target_rootfs_path + "/" + target_name;
                std::filesystem::copy(target_src_path, target_dest_path);
            }
        }
    }

    static void TearDownTestSuite()
    {
        for (std::string target : ArionMultiarchTest::targets)
        {
            for (std::string arch : ArionMultiarchTest::archs)
            {
                std::string arion_root_path = std::string(ARION_ROOT_PATH);
                std::string rootfs_path = arion_root_path + "/rootfs/" + arch + "/rootfs";
                std::string target_rootfs_path = rootfs_path + "/root/" + target;
                if (std::filesystem::exists(target_rootfs_path) && std::filesystem::is_directory(target_rootfs_path))
                    std::filesystem::remove_all(target_rootfs_path);
            }
        }
        ArionMultiarchTest::targets.clear();
        ArionMultiarchTest::archs.clear();
    }
};

#endif // ARION_TEST_COMMON_HPP
