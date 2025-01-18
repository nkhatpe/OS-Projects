/*
 * Dynamically Mapped Shared Memory Page (DMSMP) test
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
    char * page_addr = (void *)0;
    char * test_str = "CS 550 Shared Memory Page Project";
    int pid = 0, n1 = 0, n2 = 0;
    
    printf(1, "\n=========== TEST 5: test DMSMP functionality - forking before writing shared page ===========\n");
    page_addr = get_shared_page_addr(1);
    if (page_addr == (char *)0)
    {
        printf(1, "Page address NULL. Test FAILED!\n");
        exit();
    }
          
    printf(1, "Parent: creating child process ...\n");
    pid = fork();
    if (pid < 0)
    {
        printf(1, "fork() failed!\n");
        return 0;
    }
    else if (pid == 0) // child
    {
        n1 = get_free_frame_cnt();
        printf(1, "Child: free frame count = %d\n", n1);

        printf(1, "Child: writing content to the DMSMP ...\n");
        memset(page_addr, 0, PGSIZE);
        strcpy(page_addr, test_str);

        n2 = get_free_frame_cnt();
        printf(1, "Child: free frame count = %d ... %s\n", n2, n1 - n2 == 1 ? "***PASSED***" : "***FAILED***");

        sleep(200);
    }
    else // parent
    {
        sleep(100);

        printf(1, "Parent: reading from the SMSP and compare ...");
        strcmp(test_str, page_addr) == 0 ? printf(1, "***PASSED***\n") : printf(1, "***FAILED***\n");
        printf(1, "\n");
            
        wait();
        
        printf(1, "\n=========== TEST 6: manually shut down the VM with \"shutdown\", there should be no error ===========\n");
    }

    exit();
}

