#include <iostream>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    pid_t pid = fork();
    if (pid < 0) {
        std::cerr << "Fork failed!" << std::endl;
        return 1;
    } else if (pid == 0)
        std::cout << "CHILD - PID: " << std::hex << +getpid() << " PPID: " << std::hex << +getppid() << std::endl;
    else {
        wait(nullptr);
        std::cout << "PARENT - PID: " << std::hex << +getpid() << " CHILD PID: " << std::hex << +pid << std::endl;
    }
    return 0;
}