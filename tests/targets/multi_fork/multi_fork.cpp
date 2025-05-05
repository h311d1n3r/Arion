#include <iostream>
#include <unistd.h>
#include <sys/wait.h>

int main()
{
    for(uint8_t i = 0; i < 3; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            std::cerr << "Fork failed!" << std::endl;
            return 1;
        } else if (pid == 0) {
            std::cout << std::hex << +getpid() << std::endl;
            exit(0);
        } else {
            wait(nullptr);
            std::cout << std::hex << +pid << std::endl;
        }
    }
    return 0;
}
