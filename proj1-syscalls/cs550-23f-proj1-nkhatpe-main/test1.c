#include "types.h"
#include "user.h"

int main() {
    // Reseting the counts at the beginning
    reset_syscall_count();

    int pid = fork();
    if (pid > 0) {
        // Parent process
        printf(1, "Parent process is running. Child process ID: %d\n", pid);
        pid = wait();
        pid = wait();
        printf(1, "Child %d is done.\n", pid);
    } else if (pid == 0) {
        // Child process
        printf(1, "Child process is running.\n");
        exit();
    } else {
        // Fork error
        printf(1, "Fork error.\n");
    }

    // Get the counts after calls
    int fork_count = get_syscall_count(0);
    int wait_count = get_syscall_count(1);
    int exit_count = get_syscall_count(2);

    printf(1, "After calls: fork=%d, wait=%d, exit=%d\n", fork_count, wait_count, exit_count);

    // Verifying that counts have been reset
    reset_syscall_count();

    fork_count = get_syscall_count(0);
    wait_count = get_syscall_count(1);
    exit_count = get_syscall_count(2);

    printf(1, "After reset: fork=%d, wait=%d, exit=%d\n", fork_count, wait_count, exit_count);

    // Checking if counts are back to zero
    if (fork_count == 0 && wait_count == 0 && exit_count == 0) {
        printf(1, "Counts have been successfully reset to zero.\n");
    } else {
        printf(1, "Error: Counts were not reset to zero.\n");
    }

    exit();
}

