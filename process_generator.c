#include "headers.h"

void clearResources(int);
struct processData processnow;
int schedulingtype, q;

void readfile(struct Queue *processqueue);
void askuser();

int main(int argc, char *argv[])
{
    signal(SIGINT, clearResources);
    int schpid;
    key_t mqkey = ftok("process_generator.c", 1);
    key_t message_queue_id = msgget(mqkey, IPC_CREAT | 0644);
    struct Queue *processqueue = createQueue(1000);
    // TODO Initialization
    readfile(processqueue); // 1. Read the input files.

    askuser(); // 2. Ask the user for the chosen scheduling algorithm and its parameters, if there are any.

    // 3. Initiate and create the scheduler and clock processes.
    int clkpid = fork();
    if (clkpid == 0)
    {
        char *const parmList[] = {"clk.out", NULL};
        execv("clk.out", parmList);
        printf("Error\n");
    }
    else
    {
        initClk();

        schpid = fork();
        if (schpid == 0)
        {
            char st[5];
            if (schedulingtype == 1)
                sprintf(st, "%s", "HPF");
            else if (schedulingtype == 2)
                sprintf(st, "%s", "SRTN");
            else
                sprintf(st, "%d", q);

            char *const parmList[] = {"scheduler.out", st, NULL};
            execv("scheduler.out", parmList);
            printf("Error\n");
        }
    }
    // 4. Use this function after creating the clock process to initialize clock
    // To get time use this
    // TODO Generation Main Loop

    struct msgbuff mess;
    while (!isEmpty(processqueue))
    {
        if (front(processqueue).arrivaltime == getClk())
        {
            struct processData item_now = dequeue(processqueue);
            mess.mtype = 1;
            mess.p = item_now;
            kill(schpid, SIGUSR1);
            msgsnd(message_queue_id, &mess, sizeof(mess) - sizeof(mess.mtype), !IPC_NOWAIT);
        }
    }
    struct processData item_now = dequeue(processqueue);
    mess.p = item_now;
    printf("Sending %ld\n", item_now.id);
    msgsnd(message_queue_id, &mess, sizeof(mess) - sizeof(mess.mtype), !IPC_NOWAIT);
    kill(schpid, SIGUSR1);
    raise(SIGSTOP);
    // 5. Create a data structure for processes and provide it with its parameters.
    // 6. Send the information to the scheduler at the appropriate time.
    // 7. Clear clock resources
    destroyClk(0);
}

void clearResources(int signum)
{
    /*key_t mqkey = ftok("process_generator.c", 1);
    key_t message_queue_id = msgget(mqkey, 0644);
    msgctl(message_queue_id, IPC_RMID, (struct msqid_ds *)0);
    destroyClk(true);
    //TODO Clears all resources in case of interruption*/
}

void readfile(struct Queue *processqueue)
{
    char line[260];
    char *buf;
    FILE *in_file = fopen("processes.txt", "r"); // read

    if (!in_file)
    {
        printf("Error in reading input\n");
        exit(-1);
    }

    while (fgets(line, 260, in_file))
    {
        if (line[0] != '#')
        {
            buf = strtok(line, "\t\n");
            if (buf)
                processnow.id = strtol(buf, &buf, 10);
            buf = strtok(NULL, "\t\n");
            if (buf)
                processnow.arrivaltime = strtol(buf, &buf, 10);
            buf = strtok(NULL, "\t\n");
            if (buf)
                processnow.runningtime = strtol(buf, &buf, 10);
            buf = strtok(NULL, "\t\n");
            if (buf)
                processnow.priority = strtol(buf, &buf, 10);
            buf = strtok(NULL, "\t\n");
            if (buf)
                processnow.memsize = strtol(buf, &buf, 10);
            processnow.remainingtime = processnow.runningtime;
            enqueue(processqueue, processnow);
        }
    }

    fclose(in_file);
}

void askuser()
{
    printf("Choose scheduling algorithm: (1 HPF):(2 SRTN):(3 RR)\n");
    int N;
    scanf("%d", &N);
    while (N != 1 && N != 2 && N != 3)
    {
        printf("Error choose one of these choices: (1 HPF):(2 SRTN):(3 RR)\n");
        scanf("%d", &N);
    }
    if (N == 3)
    {
        printf("Enter quantum size:\n");
        scanf("%d", &q);
    }
    schedulingtype = N;
}