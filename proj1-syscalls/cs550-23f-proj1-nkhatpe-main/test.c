#include "types.h"
#include "user.h"

// Function to test syscall counts with a specified loop count
void test_loop(int loop) {
    int fork_count = 0, wait_count = 0, exit_count = 0;
    
    // Reset syscall counts at the beginning
    reset_syscall_count();
    
    // Perform the specified number of fork-wait-exit operations
    for (int i = 0; i < loop; i++) {
        int fret = fork();
        if (fret < 0) {
            printf(1, "fork failed\n");
            exit();
        } else if (fret == 0) {
            exit();
        } else {
            wait();
        }
    }
    
    // Get syscall counts after the loop
    fork_count = get_syscall_count(0);
    wait_count = get_syscall_count(1);
    exit_count = get_syscall_count(2);
    
    // Print expected and actual results
    printf(1, "Expected result: fork[%d] wait[%d] exit[%d]\n", loop, loop, loop);
    printf(1, "Actual result: fork[%d] wait[%d] exit[%d]\n", fork_count, wait_count, exit_count);
}

int main(int argc, char *argv[]) {
    printf(1, ">>> Test 1:\n");
    test_loop(5); // Test with a loop count of 5
    
    printf(1, ">>> Test 2:\n");
    test_loop(10); // Test with a loop count of 10
    
    // Add more tests with different loop counts if needed
    
    exit();
}

