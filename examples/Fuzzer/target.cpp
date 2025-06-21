#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>

int main()
{
    char buf[8];
    printf("Input: ");
    syscall(__NR_read, fileno(stdin), buf, 32);
    printf("Your message: %s\n", buf);
    return 0;
}
