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

std::string inline str_to_uppercase(const std::string &input)
{
    std::string result = input;
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) { return std::toupper(c); });
    return result;
}

std::string inline str_to_lowercase(const std::string &input)
{
    std::string result = input;
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) { return std::tolower(c); });
    return result;
}

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

std::string inline prot_flags_to_str(arion::PROT_FLAGS flags)
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

std::string inline demangle_cxx_symbol(std::string mangled_name)
{
    int status = 0;
    std::unique_ptr<char, decltype(&std::free)> demangled(
        abi::__cxa_demangle(mangled_name.c_str(), nullptr, nullptr, &status), &std::free);
    return (!status) ? demangled.get() : mangled_name;
}

std::string inline strip_str(const std::string &str)
{
    auto start = std::find_if_not(str.begin(), str.end(), [](unsigned char c) { return std::isspace(c); });

    auto end = std::find_if_not(str.rbegin(), str.rend(), [](unsigned char c) { return std::isspace(c); }).base();

    return (start < end ? std::string(start, end) : std::string());
}

std::string inline ipv4_addr_to_string(in_addr_t addr)
{
    char buf[INET_ADDRSTRLEN];
    if (!inet_ntop(AF_INET, &addr, buf, INET_ADDRSTRLEN))
        return std::string("");
    return std::string(buf);
}

std::string inline ipv6_addr_to_string(const uint8_t addr[16])
{
    char buf[INET6_ADDRSTRLEN];
    if (!inet_ntop(AF_INET6, addr, buf, INET6_ADDRSTRLEN))
        return std::string("");
    return std::string(buf);
}

std::string inline name_from_path(const std::string &path)
{
    size_t pos = path.find_last_of('/');
    if (pos != std::string::npos)
    {
        return path.substr(pos + 1);
    }
    return path;
}

#endif // ARION_CONVERT_UTILS_HPP
