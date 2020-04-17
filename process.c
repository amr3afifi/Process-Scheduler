#include "headers.h"

/* Modify this file as needed*/

long remainingtime;
int timetosleep;
int timeslept;
void stopping(int signum);
void resuming(int signum);
int main(int agrc, char *argv[])
{
    signal(SIGTSTP, stopping);
    signal(SIGCONT, resuming);
    initClk();
    timeslept = 0;
    timetosleep = 0;
    remainingtime = strtol(argv[1], &argv[1], 10);
    int starttime = getClk();
    int runtime = remainingtime;
    int current_clock = getClk();
    while (remainingtime > 0)
    {
        if (current_clock < getClk())
        {

            remainingtime = runtime - (getClk() - starttime - timeslept);
            printf("P%d: Remaining Time= %ld \n", getpid(), remainingtime);
            current_clock = getClk();
        }
    }
    kill(getppid(), SIGUSR2);
    destroyClk(false);

    return 0;
}

void stopping(int signum)
{
    timetosleep = getClk();
    raise(SIGSTOP);
}
void resuming(int signum)
{
    timeslept += getClk() - timetosleep;
    printf("I slept at:%d\n", timetosleep);
    printf("And woke up at:%d\n", getClk());
}
