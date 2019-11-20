#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <wait.h>

int main(int argc, char *argv[]) {
    if (1 == argc) {
        puts("expect at least one argument");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();
    if (0 == pid) {
        // child
        execvp(argv[1], argv + 1);
        puts("exec finished");
    } else {
        // parent
        struct timespec tp;
        clock_gettime(CLOCK_REALTIME, &tp);
        signed long sec = tp.tv_sec;
        signed long nsec = tp.tv_nsec;

        int stat_loc;
        wait(&stat_loc);

        clock_gettime(CLOCK_REALTIME, &tp);
        sec = tp.tv_sec - sec;
        if (tp.tv_nsec < nsec) {
            sec--;
            tp.tv_nsec += 1e9;
        }
        nsec = tp.tv_nsec - nsec;

        signed long hour, min, msec;
        hour = sec / 3600;
        sec -= hour * 3600;
        min = sec / 60;
        sec -= min * 60;
        msec = nsec / 1000;
        nsec -= msec * 1000;
        printf("%ld小时\t%ld分\t%ld秒\t%ld毫秒\t%ld微秒\n", hour, min, sec, msec, nsec);
        exit(EXIT_SUCCESS);
    }
}
