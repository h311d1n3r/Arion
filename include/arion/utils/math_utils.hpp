#ifndef ARION_MATH_UTILS_HPP
#define ARION_MATH_UTILS_HPP

#include <arion/common/global_defs.hpp>
#include <vector>

namespace arion
{

/**
 * Generates a vector of cryptographically secure pseudo-random bytes.
 * @param[in] n The number of random bytes to generate.
 * @return A `std::vector<BYTE>` containing the requested number of random bytes.
 */
std::vector<BYTE> gen_random_bytes(std::size_t n);

}; // namespace arion

#endif // ARION_MATH_UTILS_HPP
