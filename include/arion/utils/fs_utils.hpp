#ifndef ARION_FS_UTILS_HPP
#define ARION_FS_UTILS_HPP

#include <arion/common/global_defs.hpp>
#include <functional>
#include <string>

namespace arion
{

using RD_BIN_CALLBACK = std::function<void(std::array<BYTE, ARION_BUF_SZ> buf, ADDR off, size_t sz)>;

void read_bin_file(std::string file_path, ADDR off, size_t sz, RD_BIN_CALLBACK callback);
std::string gen_tmp_path();
std::string md5_hash_file(std::string file_path);
bool check_fd_status(int fd, short int &revents);

} // namespace arion

#endif // ARION_FS_UTILS_HPP
