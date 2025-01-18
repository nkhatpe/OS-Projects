#include "types.h"
#include "user.h"

#define P_LOOP_CNT 0x10000000
#define C_LOOP_CNT 0x20000000

unsigned int avoid_optm = 0; // a variable used to avoid compiler optimization

void do_parent(void)
{
    unsigned int cnt = 0;
    unsigned int tmp = 0;

    while (cnt < P_LOOP_CNT)
    {
        tmp += cnt;
        cnt++;
    }

    avoid_optm = tmp;
}

void do_child(void)
{
    unsigned int cnt = 0;
    unsigned int tmp = 0;

    while (cnt < C_LOOP_CNT)
    {
        tmp += cnt;
        cnt++;
    }

    avoid_optm = tmp;
}

void run_test_case(const char *test_name, int scheduler_type)
{
    printf(1, "%s\n", test_name);

    set_sched(scheduler_type);

    int pid1 = fork();
    if (pid1 == 0)
    {
        do_child();
        exit();
    }
    else
    {
        int pid2 = fork();
        if (pid2 == 0)
        {
            do_child();
            exit();
        }
        else
        {
            int pid3 = fork();
            if (pid3 == 0)
            {
                do_child();
                exit();
            }
            else
            {
                printf(1, "Parent (pid %d) has %d tickets.\n", getpid(), tickets_owned(getpid()));
                printf(1, "Child1 (pid %d) has %d tickets.\n", pid1, tickets_owned(pid1));
                printf(1, "Child2 (pid %d) has %d tickets.\n", pid2, tickets_owned(pid2));
                printf(1, "Child3 (pid %d) has %d tickets.\n", pid3, tickets_owned(pid3));
                do_parent();
                wait();
                wait();
                wait();
            }
        }
    }
    printf(1, "\n");
}

int main(int argc, char *argv[])
{
    enable_sched_trace(1);

    /* ---------------- start: add your test code ------------------- */
    
    set_sched(1);

    // Test Case 1: Round-Robin Scheduler, 3 Child Processes
    run_test_case("Test Case 1: Round-Robin Scheduler, 3 Child Processes", 0);

    // Test Case 2: Stride Scheduler, 3 Child Processes, No Ticket Transfer
    run_test_case("Test Case 2: Stride Scheduler, 3 Child Processes, No Ticket Transfer", 1);

    // Test Case 3: Stride Scheduler, Testing Return Values of Transfer Tickets
    run_test_case("Test Case 3: Stride Scheduler, Testing Return Values of Transfer Tickets", 1);

    /* ---------------- end: add your test code ------------------- */

    enable_sched_trace(0);

    exit();
}

