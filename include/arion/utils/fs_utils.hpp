#ifndef ARION_FS_UTILS_HPP
#define ARION_FS_UTILS_HPP

#include <arion/common/global_defs.hpp>
#include <functional>
#include <string>

using RD_BIN_CALLBACK = std::function<void(std::array<arion::BYTE, ARION_BUF_SZ> buf, arion::ADDR off, size_t sz)>;

void read_bin_file(std::string file_path, arion::ADDR off, size_t sz, RD_BIN_CALLBACK callback);
std::string gen_tmp_path();
std::string md5_hash_file(std::string file_path);

#endif // ARION_FS_UTILS_HPP
