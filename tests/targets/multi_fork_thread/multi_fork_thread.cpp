#include <iostream>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

void thread_function(int thread_id)
{
    std::cout << "A";
}

void create_threads()
{
    std::thread threads[3];
    for (int i = 0; i < 3; ++i)
    {
        threads[i] = std::thread(thread_function, i);
    }
    for (int i = 0; i < 3; ++i)
    {
        threads[i].join();
    }
}

int main()
{
    const int forks_n = 3;
    for (int i = 0; i < forks_n; ++i)
    {
        pid_t pid = fork();
        if (!pid)
        {
            create_threads();
            return 0;
        }
    }
    for (int i = 0; i < forks_n; ++i)
        wait(nullptr);
    std::cout << std::endl;
    return 0;
}
