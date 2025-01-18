/*
 * Statically Mapped Shared Memory Page (SMSMP) test
 */

#include "types.h"
#include "user.h"

// ***ATTETION***: The following stub function needs to be commented out 
//                 before testing your implementation.
/*#define STUB_FUNC
#ifdef STUB_FUNC
char * get_shared_page_addr(int type)
{
	return 0;
}
#endif*/

#define PGSIZE 4096

int
main(int argc, char *argv[])
{
    char * page_addr = 0;
    char * test_str = "CS 550 Shared Memory Page Project";
    int pid = 0, n1 = 0, n2 = 0;
    
    printf(1, "\n=========== TEST 1: get the statically mapped shared memory page (SMSMP) address ===========\n");
    page_addr = get_shared_page_addr(0);
    printf(1, "0x%x ...", page_addr);
    page_addr == (char *)0x3000 ? printf(1, "***PASSED***\n") : printf(1, "***FAILED***\n");
    
    printf(1, "\n=========== TEST 2: test SMSMP functionality ===========\n");
    printf(1, "Parent: creating a child process ...\n");
    pid = fork();
    if (pid < 0)
    {
        printf(1, "fork() failed!\n");
        exit();
    }
    else if (pid == 0) // child
    {
        if (page_addr == (char *)0)
        {
            printf(1, "Page address NULL. Test FAILED!\n");
        }
        else
        {
            n1 = get_free_frame_cnt();
            printf(1, "Child: free frame count = %d\n", n1);

            printf(1, "Child: writing content to the SMSMP ...\n");
            memset(page_addr, 0, PGSIZE);
            strcpy(page_addr, test_str);

            n2 = get_free_frame_cnt();
            printf(1, "Child: free frame count = %d ... %s\n", n2, n1 == n2 ? "***PASSED***\b" : "***FAILED***\n");
        }
    }
    else // parent
    {
        wait();
        printf(1, "Parent: reading from the SMSP and compare ...");
        strcmp(test_str, page_addr) == 0 ? printf(1, "***PASSED***\n") : printf(1, "***FAILED***\n");
        printf(1, "\n");
    }

    exit();
}

