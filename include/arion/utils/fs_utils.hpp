#ifndef ARION_FS_UTILS_HPP
#define ARION_FS_UTILS_HPP

#include <arion/common/global_defs.hpp>
#include <functional>
#include <string>

namespace arion
{

/// Callback type used for processing chunks of binary data read from a file.
using RD_BIN_CALLBACK = std::function<void(std::array<BYTE, ARION_BUF_SZ> buf, ADDR off, size_t sz)>;

/**
 * Reads chunks of a binary file and passes them to a callback function.
 * This function reads the file starting at a specific offset for a specific size.
 * @param[in] file_path Path to the binary file.
 * @param[in] off The starting byte offset in the file to begin reading.
 * @param[in] sz The total number of bytes to read from the file.
 * @param[in] callback The callback function to receive and process each chunk of data.
 */
void read_bin_file(std::string file_path, ADDR off, size_t sz, RD_BIN_CALLBACK callback);
/**
 * Generates a unique temporary file path.
 * @return A string containing the path to a newly generated temporary file.
 */
std::string gen_tmp_path();
/**
 * Calculates the MD5 hash checksum of a given file.
 * @param[in] file_path Path to the file to be hashed.
 * @return The MD5 hash of the file content as a hexadecimal string.
 */
std::string md5_hash_file(std::string file_path);
/**
 * Checks the I/O status (readiness) of a given file descriptor using `poll()`.
 * @param[in] fd The file descriptor to check.
 * @param[out] revents The returned events bitmap (e.g., POLLIN, POLLOUT).
 * @return `true` if the poll was successful, `false` otherwise.
 */
bool check_fd_status(int fd, short int &revents);

} // namespace arion

#endif // ARION_FS_UTILS_HPP
