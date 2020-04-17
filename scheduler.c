#include "headers.h"

void inserthandler(int signum);
void deletehandler(int signum);

long initialaddress;
char *schtype;
struct pcb *tablep;
struct memblock *memory;
int allarrived = 0;
struct msgbuff mess;
long runningid;
int clockbeforesleep;
int runningpindex;
FILE *out_sch;
FILE *out_mem;
// key_t message_queue_id;
// key_t mqkey;

int main(int argc, char *argv[])
{
    initClk();
    signal(SIGUSR1, inserthandler);
    signal(SIGUSR2, deletehandler);

    memory = CreateMemTree();
    printf("%d %d %d %d\n", (int)memory->left, (int)memory->right, memory->value, memory->fill);
    tablep = createpcb();
    clockbeforesleep = getClk();
    schtype = argv[1];
    out_sch = fopen("schedulerlog", "w");                                              // write only
    out_mem = fopen("memlog", "w");                                                    // write only
    fprintf(out_sch, "#At time x process y started arr w total x remain y wait k \n"); // write to file
    fprintf(out_mem, "#At time x allocated y bytes for process x from i to j \n");
    if (out_sch == NULL || out_mem == NULL)
    {
        printf("Error! Could not open output file\n");
        exit(-1);
    }
    printf("Scheduler: Type= %s\n", schtype);
    if (strstr(schtype, "HPF") || strstr(schtype, "SRTN"))
    {
        while (true)
        {
            sleep(INT_MAX);
        }
    }
    else
    {
        int q = strtol(schtype, &schtype, 10);
        runningpindex = 0;
        while (true)
        {
            if (tablep->count != 0)
            {
                kill(getbypos(tablep, runningpindex)->p.syspid, SIGTSTP);
                runningpindex = (runningpindex + 1) % tablep->count;
                kill(getbypos(tablep, runningpindex)->p.syspid, SIGCONT);
                printf("%d\n", runningpindex);
                printpcb(tablep);
            }
            sleep(q);
        }
    }
    //TODO implement the scheduler :)
    //upon termination release the clock resources
}

void inserthandler(int signum)
{
    key_t mqkey = ftok("process_generator.c", 1);
    key_t message_queue_id = msgget(mqkey, 0644);
    int rec_val;
    rec_val = msgrcv(message_queue_id, &mess, sizeof(mess) - sizeof(mess.mtype), 0, !IPC_NOWAIT);
    printf("Scheduler: Received P%ld Arr=%ld Run=%ld Priority:%ld mem: %ld\n", mess.p.id, mess.p.arrivaltime, mess.p.runningtime, mess.p.priority, mess.p.memsize);

    if (mess.p.id == -1)
    {
        msgctl(message_queue_id, IPC_RMID, (struct msqid_ds *)0);
        allarrived = 1;
    }

    else if (strstr(schtype, "HPF") || strstr(schtype, "SRTN"))
    {
        int pid = fork();
        if (pid == 0)
        {
            char running_time[12];

            printf("Process: P%ld Arr=%ld Run=%ld Priority:%ld\n", mess.p.id, mess.p.arrivaltime, mess.p.runningtime, mess.p.priority);
            sprintf(running_time, "%ld", mess.p.runningtime);

            char *const parmList[] = {"process.out", running_time, NULL};
            raise(SIGSTOP);
            execv("process.out", parmList);
        }
        mess.p.syspid = pid;
        if (tablep->count != 0 && getClk() > clockbeforesleep)
        {
            getbypos(tablep, 0)->p.remainingtime -= getClk() - clockbeforesleep;
        }
        clockbeforesleep = getClk();
        if (strstr(schtype, "SRTN") && tablep->count != 0)
        {
            struct processData printingonly = getbypos(tablep, 0)->p;
            //  kill(getbypos(tablep, 0)->p.syspid, SIGSTOP);
            int x = getClk();
            kill(printingonly.syspid, SIGTSTP);
            fprintf(out_sch, "At time %d process %ld stopped arr %ld total %ld remain %ld wait %ld \n", x, printingonly.id, printingonly.arrivaltime, printingonly.runningtime, printingonly.remainingtime, (long)(x - printingonly.starttime - (printingonly.runningtime - printingonly.remainingtime)));
        }
        mess.p.starttime = getClk();
        //printf("------------- %d %d %d %d ------------------\n", (int)memory->left, (int)memory->right, memory->value, memory->fill);
        mess.p.startddress = getmem(mess.p.memsize, memory);
        //printf("Scheduler entered --------------------------------------------------------------\n");

        insert(tablep, mess.p, schtype);
        if (strstr(schtype, "SRTN") && tablep->count != 1 && tablep->count != 0)
        {
            int stat;
            waitpid(getbypos(tablep, 0)->p.syspid, &stat, WUNTRACED);
            kill(getbypos(tablep, 0)->p.syspid, SIGCONT);
            runningid = getbypos(tablep, 0)->p.id;
            struct processData printingonly = getprocess(tablep, runningid);
            // if (printingonly.remainingtime == printingonly.runningtime)
            if (printingonly.runningtime == printingonly.remainingtime)
                //fprintf(out_mem, "At time %d allocated %ld bytes for process %ld from %ld to %ld \n", getClk(), printingonly.memsize, printingonly.id, printingonly.startddress, printingonly.startddress + roundUp((int)printingonly.memsize) - 1);
                fprintf(out_sch, "At time %d process %ld started arr %ld total %ld remain %ld wait %ld \n", getClk(), printingonly.id, printingonly.arrivaltime, printingonly.runningtime, printingonly.runningtime, getClk() - printingonly.starttime - (printingonly.runningtime - printingonly.remainingtime));
            //else
            //  fprintf(out_sch, "At time %d process %ld resumed arr %ld total %ld remain %ld wait %ld \n", getClk(), printingonly.id, printingonly.arrivaltime, printingonly.runningtime, printingonly.remainingtime, getClk() - printingonly.starttime - (printingonly.runningtime - printingonly.remainingtime));
        }
        printpcb(tablep);

        if (tablep->count == 1)
        {
            int stat;
            waitpid(pid, &stat, WUNTRACED);
            kill(pid, SIGCONT);
            if (mess.p.starttime == getClk())
            {
                if (mess.p.remainingtime == mess.p.runningtime)
                    //fprintf(out_mem, "At time %d allocated %ld bytes for process %ld from %ld to %ld \n", getClk(), mess.p.memsize, mess.p.id, mess.p.startddress, mess.p.startddress + roundUp((int)mess.p.memsize) - 1);
                    fprintf(out_sch, "At time %d process %ld started arr %ld total %ld remain %ld wait %ld \n", getClk(), mess.p.id, mess.p.arrivaltime, mess.p.runningtime, mess.p.runningtime, getClk() - mess.p.arrivaltime);
            } // else
            //fprintf(out_sch, "At time %d process %ld stopped arr %ld total %ld remain wait %ld \n", x, printingonly.id, printingonly.arrivaltime, printingonly.runningtime, printingonly.remainingtime, (x - mess.p.arrivaltime));

            runningid = mess.p.id;
        }
    }

    else
    {
        int pid = fork();
        if (pid == 0)
        {
            char running_time[12];

            printf("Process: P%ld Arr=%ld Run=%ld Priority:%ld\n", mess.p.id, mess.p.arrivaltime, mess.p.runningtime, mess.p.priority);
            sprintf(running_time, "%ld", mess.p.runningtime);

            char *const parmList[] = {"process.out", running_time, NULL};
            raise(SIGSTOP);
            execv("process.out", parmList);
        }
        mess.p.syspid = pid;
        mess.p.startddress = getmem(mess.p.memsize, memory);
        fprintf(out_mem, "At time %d allocated %ld bytes for process %ld from %ld to %ld \n", getClk(), mess.p.memsize, mess.p.id, mess.p.startddress, mess.p.startddress + roundUp((int)mess.p.memsize) - 1);

        insert(tablep, mess.p, schtype);
        if (tablep->count == 1)
        {
            int stat;
            waitpid(pid, &stat, WUNTRACED);
            kill(pid, SIGCONT);
            //fprintf(out_mem, "At time %d allocated %ld bytes for process %ld from %ld to %ld \n", getClk(), mess.p.memsize, mess.p.id, mess.p.startddress, mess.p.startddress + roundUp((int)mess.p.memsize) - 1);

            fprintf(out_sch, "At time %d process %ld started arr %ld total %ld remain wait %ld \n", getClk(), mess.p.id, mess.p.arrivaltime, mess.p.runningtime, mess.p.runningtime, getClk() - mess.p.arrivaltime);
            runningid = mess.p.id;
            runningpindex = 0;
        }
    }
}

void deletehandler(int signum)
{
    if (strstr(schtype, "HPF") || strstr(schtype, "SRTN"))
    {
        struct processData printingonly = getprocess(tablep, getbypos(tablep, runningpindex)->p.id);
        if (printingonly.id != -1)
        {
            fprintf(out_sch, "At time %d process %ld finished arr %ld total %ld remain %d wait %d TA %d WTA %.2f \n", getClk(), printingonly.id, printingonly.arrivaltime, printingonly.runningtime, 0, (getClk() - printingonly.arrivaltime - printingonly.runningtime), getClk() - printingonly.arrivaltime, ((float)getClk() - (float)printingonly.arrivaltime) / ((float)printingonly.runningtime));
            fprintf(out_mem, "At time %d freed %ld bytes for process %ld from %ld to %ld \n", getClk(), printingonly.memsize, printingonly.id, printingonly.startddress, printingonly.startddress + roundUp((int)printingonly.memsize) - 1);
        }
        //deallocatemem((getprocess(tablep, runningid).startddress), getprocess(tablep, runningid).memsize, memory);
        deletebyid(tablep, runningid);
        if (tablep->count != 0)
        {
            kill(getbypos(tablep, 0)->p.syspid, SIGCONT);
            runningid = getbypos(tablep, 0)->p.id;
            struct processData printingonly = getprocess(tablep, runningid);
            if (strstr(schtype, "HPF") || getbypos(tablep, 0)->p.remainingtime == getbypos(tablep, 0)->p.runningtime)
            {
                fprintf(out_sch, "At time %d process %ld started arr %ld total %ld remain %ld wait %ld \n", getClk(), printingonly.id, printingonly.arrivaltime, printingonly.runningtime, printingonly.runningtime, getClk() - printingonly.starttime);
                //fprintf(out_mem, "At time %d allocated %ld bytes for process %ld from %ld to %ld \n", getClk(), printingonly.memsize, printingonly.id, printingonly.startddress, printingonly.startddress + roundUp((int)printingonly.memsize) - 1);
            }
            else
                fprintf(out_sch, "At time %d process %ld resumed arr %ld total %ld remain %ld wait %ld \n", getClk(), printingonly.id, printingonly.arrivaltime, printingonly.runningtime, printingonly.runningtime - printingonly.remainingtime, getClk() - printingonly.starttime - (printingonly.runningtime - printingonly.remainingtime));
        }

        printpcb(tablep);
    }

    else
    {
        struct processData printingonly = getprocess(tablep, getbypos(tablep, runningpindex)->p.id);
        if (printingonly.id != -1)
        {
            fprintf(out_mem, "At time %d freed %ld bytes for process %ld from %ld to %ld \n", getClk(), printingonly.memsize, printingonly.id, printingonly.startddress, printingonly.startddress + roundUp((int)printingonly.memsize) - 1);
            fprintf(out_sch, "At time %d process %ld finished arr %ld total %ld remain %d wait %d TA %d WTA %.2f \n", getClk(), printingonly.id, printingonly.arrivaltime, printingonly.runningtime, 0, (getClk() - printingonly.arrivaltime - printingonly.runningtime), getClk() - printingonly.arrivaltime, ((float)getClk() - (float)printingonly.arrivaltime) / ((float)printingonly.runningtime));
        }
        //deallocatemem(getbypos(tablep, runningpindex)->p.startddress, getbypos(tablep, runningpindex)->p.memsize, memory);
        deletebyid(tablep, getbypos(tablep, runningpindex)->p.id);
        runningpindex = (runningpindex - 1) % tablep->count;
        if (tablep->count != 0)
        {
            kill(getbypos(tablep, runningpindex)->p.syspid, SIGCONT);
            runningid = getbypos(tablep, 0)->p.id;
            struct processData printingonly = getprocess(tablep, runningid);
            fprintf(out_sch, "At time %d process %ld resumed arr %ld total %ld remain %ld wait %ld \n", getClk(), printingonly.id, printingonly.arrivaltime, printingonly.runningtime, printingonly.runningtime - printingonly.remainingtime, getClk() - printingonly.starttime - (printingonly.runningtime - printingonly.remainingtime));
        }
        printpcb(tablep);
    }

    if (allarrived == 1 && tablep->count == 0)
    {
        FILE *ut = fopen("ut", "w");
        fprintf(ut, "Utilization of CPU: %d", 1);
        fclose(ut);
        fclose(out_sch);
        fclose(out_mem);
        kill(getppid(), SIGCONT);
        destroyClk(1);
    }
}