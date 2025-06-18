#include <unistd.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <linux/time.h>

int main() {
    struct timespec ts;
    char buf[64];

    syscall(SYS_clock_gettime, CLOCK_REALTIME, &ts);

    int len = snprintf(buf, sizeof(buf), "%ld\n", ts.tv_sec);

    syscall(SYS_write, 1, buf, len);
    syscall(SYS_exit_group, 0);
}
