#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "stdio.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

int sys_shutdown(void)
{
  /* Either of the following will work. Does not harm to put them together. */
  outw(0xB004, 0x0|0x2000); // working for old qemu
  outw(0x604, 0x0|0x2000); // working for newer qemu
  
  return 0;
}

extern int sched_trace_enabled;
extern int sched_trace_counter;
int sys_enable_sched_trace(void)
{
  if (argint(0, &sched_trace_enabled) < 0)
  {
    cprintf("enable_sched_trace() failed!\n");
  }
  
  sched_trace_counter = 0;

  return 0;
}

extern int winner;

// Function to set the winner variable, affecting scheduling behavior.
int sys_fork_alternate_winner(void)
{
	int n;

	// Get the argument 'n' from the system call.
	if (argint(0, &n) < 0)
	{
		return -1; // Return an error if the argument retrieval fails.
	}

	// Set the winner variable based on the value of 'n'.
	if (n == 1)
	{
		winner = 1; // Set the winner to 1, altering the fork behavior.
	}
	if (n == 0)
	{
		winner = 0; // Set the winner to 0,
	}

	return 0; // Return 0 to indicate success.
}

extern int sched_policy;

// Function to set the scheduling policy (0 for Round Robin, 1 for Stride Scheduling).
int sys_set_sched(void)
{
	int x;

	// Get the argument 'x' from the system call.
	if (argint(0, &x) < 0)
	{
		return -1; // Return an error if the argument retrieval fails.
	}

	// Set the scheduling policy based on the value of 'x'.
	if (x == 1)
	{
		sched_policy = 1; // Set the policy to Stride Scheduling.
	}
	if (x == 0)
	{
		sched_policy = 0; // Set the policy to Round Robin.
	}

	return 0; // Return 0 to indicate success.
}

// Function to retrieve the number of tickets owned by a process with a given 'pid'.
int sys_tickets_owned(void)
{
	int pid;
	int tickets;

	// Get the 'pid' argument from the system call.
	if (argint(0, &pid) < 0)
	{
		return -1; // Return an error if the argument retrieval fails.
	}

	// Call a function to determine the number of tickets owned by the specified process.
	tickets = procs_tickets_owned(pid);

	return tickets; // Return the number of tickets owned by the process.
}

// Function to transfer a specified number of tickets to another process with 'pid'.
int sys_transfer_tickets(void)
{
	int pid, tickets, tickets_after_transfer;

	// Get the 'pid' and 'tickets' arguments from the system call.
	argint(0, &pid);
	argint(1, &tickets);

	if (tickets < 0)
	{
		return -1; // Return an error if the number of tickets to transfer is negative.
	}

	// Check if the number of tickets to transfer is greater than the current process's tickets minus 1.
	if (tickets > (myproc()->tickets - 1))
	{
		return -2; // Return an error if the transfer is not allowed.
	}

	// Call a function to perform the ticket transfer and return the resulting number of tickets.
	tickets_after_transfer = tickets_transfer(pid, tickets, myproc());

	return tickets_after_transfer; // Return the number of tickets after the transfer.
}

