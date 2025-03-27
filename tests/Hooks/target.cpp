#include <iostream>
#include <cstdlib>
#include <signal.h>
#include <sys/mman.h>

void signal_handler(int sig, siginfo_t *info, void *context) {
    std::cout << "Sighandler called" << std::endl;
    exit(0);
}

void setup_signal_handler() {
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = signal_handler;
    sigaction(SIGTRAP, &sa, nullptr);
}

int main() {
    printf("OK\n");
    setup_signal_handler();
    __asm__("int3");
    return 0;
}