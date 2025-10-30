#include <iostream>

// example binary with differents blocks input related
int main() {
    int a, b;

    std::cout << "Enter two integers: ";
    std::cin >> a >> b;

    if (a < 0 && b < 0) {
        std::cout << "Both numbers are negative.\n";
    } else if (a < 0 || b < 0) {
        std::cout << "One number is negative.\n";
    } else {
        std::cout << "Both numbers are non-negative.\n";
    }

    if (a == 0 || b == 0) {
        std::cout << "At least one number is zero.\n";
    }

    if ((a % 2 == 0) && (b % 2 == 0)) {
        std::cout << "Both numbers are even.\n";
    } else if ((a % 2 != 0) && (b % 2 != 0)) {
        std::cout << "Both numbers are odd.\n";
    } else {
        std::cout << "One number is even, one is odd.\n";
    }

    if (a > b) {
        std::cout << "a is greater than b.\n";
    } else if (a < b) {
        std::cout << "a is less than b.\n";
    } else {
        std::cout << "a is equal to b.\n";
    }

    int sum = a + b;
    std::cout << "Sum: " << sum << std::endl;

    return 0;
}
