#include <arion/utils/math_utils.hpp>
#include <random>

using namespace arion;

std::vector<BYTE> arion::gen_random_bytes(size_t n)
{
    std::vector<BYTE> ran_bytes(n);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 0xFF);

    for (size_t i = 0; i < n; i++)
    {
        ran_bytes[i] = static_cast<BYTE>(dis(gen));
    }

    return ran_bytes;
}
