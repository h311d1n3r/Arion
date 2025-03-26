#include <iostream>
#include <thread>

void child_thread() {
    std::cout << "CHILD THREAD" << std::endl;
}

int main() {
    std::thread t(child_thread);
    std::cout << "PARENT THREAD" << std::endl;
    t.join();
    return 0;
}