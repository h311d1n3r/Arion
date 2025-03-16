#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

int main()
{
    pid_t pid = fork();

    if (pid == -1)
    {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }
    else if (pid == 0)
    {
        char *args[] = {"/bin/ls", "-l", NULL};
        execve(args[0], args, NULL);

        perror("execve failed");
        exit(EXIT_FAILURE);
    }
    else
    {
        wait(NULL);
    }

    return 0;
}
