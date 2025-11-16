#ifndef ARION_CONVERT_UTILS_HPP
#define ARION_CONVERT_UTILS_HPP

#include <algorithm>
#include <arion/common/global_defs.hpp>
#include <arpa/inet.h>
#include <cxxabi.h>
#include <iomanip>
#include <memory>
#include <netinet/in.h>
#include <sstream>
#include <string>

namespace arion
{

/**
 * Converts a string to uppercase.
 * @param[in] input The string to convert.
 * @return The uppercase version of the input string.
 */
std::string inline str_to_uppercase(const std::string &input)
{
    std::string result = input;
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) { return std::toupper(c); });
    return result;
}

/**
 * Converts a string to lowercase.
 * @param[in] input The string to convert.
 * @return The lowercase version of the input string.
 */
std::string inline str_to_lowercase(const std::string &input)
{
    std::string result = input;
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) { return std::tolower(c); });
    return result;
}

/**
 * Converts an integral value to its hexadecimal string representation.
 * @tparam T The integral type of the value.
 * @param[in] value The integral value to convert.
 * @param[in] padding The width of the padding (number of characters to fill with leading zeros). Default is 0 (no
 * padding).
 * @param[in] prefix Whether to include the "0x" prefix. Default is true.
 * @return The hexadecimal string representation of the value.
 */
template <typename T> inline std::string int_to_hex(T value, uint8_t padding = 0, bool prefix = true)
{
    static_assert(std::is_integral<T>::value, "T must be an integral type.");

    std::stringstream stream;
    if (prefix)
        stream << "0x";
    if (padding)
        stream << std::setfill('0') << std::setw(padding);
    stream << std::hex << +value;
    return stream.str();
}

/**
 * Converts memory protection flags (`PROT_FLAGS`) to a simplified "rwx" string format.
 * @param[in] flags The `PROT_FLAGS` value (e.g., PROT_READ | PROT_WRITE).
 * @return A string representing the permissions (e.g., "rw-").
 */
std::string inline prot_flags_to_str(PROT_FLAGS flags)
{
    const std::string generic_flags = "rwx";
    std::stringstream ss;

    for (uint8_t bit_i = 0; bit_i < 3; bit_i++)
    {
        if ((flags >> (2 - bit_i)) & 1)
            ss << generic_flags[bit_i];
        else
            ss << "-";
    }

    return ss.str();
}

/**
 * Demangles a C++ symbol name if it is mangled.
 * @param[in] mangled_name The potentially mangled C++ symbol name.
 * @return The demangled name if successful, otherwise the original mangled name.
 */
std::string inline demangle_cxx_symbol(std::string mangled_name)
{
    int status = 0;
    std::unique_ptr<char, decltype(&std::free)> demangled(
        abi::__cxa_demangle(mangled_name.c_str(), nullptr, nullptr, &status), &std::free);
    return (!status) ? demangled.get() : mangled_name;
}

/**
 * Removes leading and trailing whitespace from a string.
 * @param[in] str The string to strip.
 * @return The stripped string.
 */
std::string inline strip_str(const std::string &str)
{
    auto start = std::find_if_not(str.begin(), str.end(), [](unsigned char c) { return std::isspace(c); });

    auto end = std::find_if_not(str.rbegin(), str.rend(), [](unsigned char c) { return std::isspace(c); }).base();

    return (start < end ? std::string(start, end) : std::string());
}

/**
 * Converts a 32-bit IPv4 address (`in_addr_t`) to a dotted-decimal string.
 * @param[in] addr The 32-bit IPv4 address.
 * @return The dotted-decimal string representation (e.g., "192.168.1.1"), or an empty string on error.
 */
std::string inline ipv4_addr_to_string(in_addr_t addr)
{
    char buf[INET_ADDRSTRLEN];
    if (!inet_ntop(AF_INET, &addr, buf, INET_ADDRSTRLEN))
        return std::string("");
    return std::string(buf);
}

/**
 * Converts a 128-bit IPv6 address (represented as a 16-byte array) to a standard string format.
 * @param[in] addr Pointer to the 16-byte IPv6 address array.
 * @return The standard string representation of the IPv6 address, or an empty string on error.
 */
std::string inline ipv6_addr_to_string(const uint8_t addr[16])
{
    char buf[INET6_ADDRSTRLEN];
    if (!inet_ntop(AF_INET6, addr, buf, INET6_ADDRSTRLEN))
        return std::string("");
    return std::string(buf);
}

/**
 * Extracts the filename/name component from a full path string.
 * @param[in] path The full path string.
 * @return The name following the last '/' character, or the original path if no '/' is found.
 */
std::string inline name_from_path(const std::string &path)
{
    size_t pos = path.find_last_of('/');
    if (pos != std::string::npos)
    {
        return path.substr(pos + 1);
    }
    return path;
}

}; // namespace arion

#endif // ARION_CONVERT_UTILS_HPP
