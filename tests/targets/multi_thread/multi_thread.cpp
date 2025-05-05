#include <iostream>
#include <thread>
#include <vector>

size_t id = 0;

void thread_func() {
    id++;
    std::cout << std::hex << +id << std::endl;
}

int main() {
    std::vector<std::thread> threads;

    for (uint8_t i = 0; i < 3; ++i) {
        threads.emplace_back(thread_func);
        threads.back().join();
        std::cout << std::hex << +id << std::endl;
    }

    return 0;
}
