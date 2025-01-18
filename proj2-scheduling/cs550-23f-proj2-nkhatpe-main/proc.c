#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

#define INFINITY __INT_MAX__

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

static struct proc *initproc;

int nextpid = 1;
int sched_trace_enabled = 0; // ZYF: for OS CPU/process project
int sched_trace_counter = 0; // ZYF: counter for print formatting
int sched_policy;	     // Declaring Variable to determine scheduling policy
int STRIDE_TOTAL_TICKETS = 100;	// total number of tickets in the Stride Scheduling policy
int winner;			// Used for alternate fork function
int counter = 0;		// Counter for alternate fork function implementation
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

// Must be called with interrupts disabled
int
cpuid() {
  return mycpu()-cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu*
mycpu(void)
{
  int apicid, i;
  
  if(readeflags()&FL_IF)
    panic("mycpu called with interrupts enabled\n");
  
  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i) {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc*
myproc(void) {
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;

  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;

  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  return p;
}

//PAGEBREAK: 32
// Set up first user process.
void userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  // Allocate a new process structure
  p = allocproc();

  // Set the initial process to the one we just allocated
  initproc = p;

  // Allocate a new kernel page directory for the process
  if ((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");

  // Initialize the user virtual memory layout with the init code
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);

  // Set the process size to the page size
  p->sz = PGSIZE;

  // Initialize the process trapframe
  memset(p->tf, 0, sizeof(*p->tf));

  // Set code and data segment selectors in the trapframe
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;

  // Set the interrupt flag in the EFLAGS register
  p->tf->eflags = FL_IF;

  // Set the stack pointer and instruction pointer in the trapframe
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  // Set the process name to "initcode"
  safestrcpy(p->name, "initcode", sizeof(p->name));

  // Set the current working directory to the root directory
  p->cwd = namei("/");

  // Acquire the process table lock before changing process state
  acquire(&ptable.lock);

  // Set the process state to RUNNABLE, allowing it to run
  p->state = RUNNABLE;

  // Calculate the number of currently running processes
  struct proc *x;
  int proc_count = 0;
  for (x = ptable.proc; x < &ptable.proc[NPROC]; x++)
  {
    if (x->state == RUNNING || x->state == RUNNABLE)
    {
      proc_count++;
    }
  }

  // Calculate the number of tickets each process should have
  int tickets_per_proc = STRIDE_TOTAL_TICKETS / proc_count;

  // Initialize stride scheduling parameters for all processes
  for (x = ptable.proc; x < &ptable.proc[NPROC]; x++)
  {
    if (x->state == RUNNING || x->state == RUNNABLE)
    {
      x->tickets = tickets_per_proc;
      x->strides = (STRIDE_TOTAL_TICKETS * 10) / x->tickets;
      x->pass = 0;
    }
    else
    {
      x->tickets = 0;
      x->strides = 0;
      x->pass = 0;
    }
  }

  // Release the process table lock
  release(&ptable.lock);
}


// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();

  sz = curproc->sz;
  if(n > 0){
    if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  curproc->sz = sz;
  switchuvm(curproc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int 
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate a new process structure for the child process.
  if ((np = allocproc()) == 0) {
    return -1; // Return -1 if process allocation fails.
  }

  // Copy the process state (page directory, size, trapframe, etc.) from the parent process.
  if ((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0) {
    // If copying the process state fails, free allocated resources and mark the child as UNUSED.
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1; // Return -1 to indicate failure.
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;

  // Clear the %eax register so that the child process returns 0.
  np->tf->eax = 0;

  // Duplicate the file descriptors and the current working directory.
  for (i = 0; i < NOFILE; i++) {
    if (curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  }
  np->cwd = idup(curproc->cwd);

  // Copy the process name from the parent process.
  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  // Get the child process's ID (pid).
  pid = np->pid;

  // Acquire the process table lock before changing process state.
  acquire(&ptable.lock);

  // Set the child process's state to RUNNABLE, allowing it to be scheduled.
  np->state = RUNNABLE;

  // Calculate the number of currently running processes.
  struct proc *p;
  int proc_count = 0;

  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
    if (p->state == RUNNING || p->state == RUNNABLE) {
      proc_count++;
    }
  }

  // Calculate the number of tickets each process should have for stride scheduling.
  int tickets_per_proc = STRIDE_TOTAL_TICKETS / proc_count;

  // Initialize stride scheduling parameters for all processes.
  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
    if (p->state == RUNNING || p->state == RUNNABLE) {
      p->tickets = tickets_per_proc;
      p->strides = (STRIDE_TOTAL_TICKETS * 10) / p->tickets;
      p->pass = 0;
    } else {
      p->tickets = 0;
      p->strides = 0;
      p->pass = 0;
    }
  }

  // Release the process table lock.
  release(&ptable.lock);

  // Check if the child process should yield the CPU.
  if (winner == 1) {
    if (counter % 2 == 1) {
      counter++;
      yield(); // Yield the CPU to another process.
    } else {
      counter++;
    }
  }

  return pid; // Return the child process's ID (pid).
}


// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if (curproc == initproc)
    panic("init exiting"); // If the initial process tries to exit, it's an error.

  // Close all open files associated with the current process.
  for (fd = 0; fd < NOFILE; fd++) {
    if (curproc->ofile[fd]) {
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();

  // Release the current working directory.
  iput(curproc->cwd);

  end_op();
  curproc->cwd = 0;

  acquire(&ptable.lock);

  // Wake up the parent process if it is waiting in the 'wait' system call.
  wakeup1(curproc->parent);

  // Pass any abandoned children (ZOMBIE state) to the 'init' process.
  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
    if (p->parent == curproc) {
      p->parent = initproc; // Set the parent to 'init' process.
      if (p->state == ZOMBIE)
        wakeup1(initproc); // Wake up 'init' if the child was in ZOMBIE state.
    }
  }

  // Set the current process state to ZOMBIE, indicating it has exited.
  curproc->state = ZOMBIE;

  // Calculate the number of currently running processes.
  struct proc *x;
  int proc_count = 0;

  for (x = ptable.proc; x < &ptable.proc[NPROC]; x++) {
    if (x->state == RUNNING || x->state == RUNNABLE) {
      proc_count++;
    }
  }

  // Calculate the number of tickets each process should have for stride scheduling.
  int tickets_per_proc = STRIDE_TOTAL_TICKETS / proc_count;

  // Initialize stride scheduling parameters for all processes.
  for (x = ptable.proc; x < &ptable.proc[NPROC]; x++) {
    if (x->state == RUNNING || x->state == RUNNABLE) {
      x->tickets = tickets_per_proc;
      x->strides = (STRIDE_TOTAL_TICKETS * 10) / x->tickets;
      x->pass = 0;
    } else {
      x->tickets = 0;
      x->strides = 0;
      x->pass = 0;
    }
  }

  sched(); // Jump into the scheduler, never to return.

  panic("zombie exit"); 
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();
  
  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void 
scheduler(void)
{
  struct proc *p;
  struct proc *min_pass_proc = 0;
  struct cpu *c = mycpu();
  c->proc = 0;

  int ran = 0; // CS 350/550: to solve the 100%-CPU-utilization-when-idling problem

  for (;;)
  {
    // Enable interrupts on this processor.
    sti();

    // Loop over process table looking for a process to run.
    acquire(&ptable.lock);

    if (sched_policy)
    {
      // Stride Scheduling Policy

      int min_pass = INFINITY;

      ran = 0;

      // Find the process with the minimum pass value (the next to run).
      for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
      {
        if (p->state != RUNNABLE)
          continue;

        if (p->pass < min_pass)
        {
          min_pass = p->pass;
          min_pass_proc = p;
        }
      }

      ran = 1;

      // Set the CPU's current process to the one with the minimum pass value.
      c->proc = min_pass_proc;
      min_pass_proc->pass = min_pass_proc->pass + min_pass_proc->strides;
      switchuvm(min_pass_proc);
      min_pass_proc->state = RUNNING;

      // Switch to the chosen process's context.
      swtch(&(c->scheduler), min_pass_proc->context);
      switchkvm();

      c->proc = 0; // Reset the CPU's current process to 0.
    }
    else
    {
      // Round Robin Scheduling Policy as it is

      ran = 0;

      for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
      {
        if (p->state != RUNNABLE)
          continue;

        ran = 1;

        // Set the CPU's current process to the one being scheduled.
        c->proc = p;
        switchuvm(p);
        p->state = RUNNING;

        // Switch to the chosen process's context.
        swtch(&(c->scheduler), p->context);
        switchkvm();

        // Process is done running for now.
        // It should have changed its p->state before coming back.
        c->proc = 0; // Reset the CPU's current process to 0.
      }
    }

    release(&ptable.lock);

    // If no process was scheduled (ran == 0), halt the CPU to save power.
    if (ran == 0)
    {
      halt();
    }
  }
}


// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(mycpu()->ncli != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = mycpu()->intena;
  swtch(&p->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  if (sched_trace_enabled)
  {
    cprintf("%d", myproc()->pid);
    
    sched_trace_counter++;
    if (sched_trace_counter % 20 == 0)
    {
      cprintf("\n");
    }
    else
    {
      cprintf(" - ");
    }
  }
    
  acquire(&ptable.lock);  //DOC: yieldlock
  myproc()->state = RUNNABLE;
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();
  
  if(p == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }
  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan)
      p->state = RUNNABLE;
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}

// Function to retrieve the number of tickets owned by a process with the specified 'pid'.
int procs_tickets_owned(int pid)
{
    struct proc *p;

    // Iterate through the process table to find the process with the matching 'pid'.
    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    {
        // Check if the 'pid' of the current process matches the specified 'pid'.
        if (p->pid == pid)
        {
            // Return the number of tickets owned by the process.
            return p->tickets;
        }
    }

    // If no process with the specified 'pid' is found, return 0 tickets (process not found or owns no tickets).
    return 0;
}

// Function to transfer a specified number of tickets from the current process to a process with the specified 'pid'.
int tickets_transfer(int pid, int tickets, struct proc *p)
{
  struct proc *x;
  int tickets_transferred = 0;

  // Iterate through the process table to find the process with the matching 'pid'.
  for (x = ptable.proc; x < &ptable.proc[NPROC]; x++)
  {
    if (x->pid == pid)
    {
      // Reset the transferred tickets count to 0 and exit the loop.
      tickets_transferred = 0;
      break;
    }
    else
    {
      // Set the transferred tickets count to -3 to indicate that the process with 'pid' was not found.
      tickets_transferred = -3;
    }
  }

  // Check if a valid process with the specified 'pid' was found.
  if (tickets_transferred == -3)
  {
    return tickets_transferred; // Return -3 to indicate that the specified process was not found.
  }

  // Update the number of tickets and stride for both the target process and the current process.
  x->tickets = x->tickets + tickets;
  x->strides = (STRIDE_TOTAL_TICKETS * 10) / x->tickets;

  p->tickets = p->tickets - tickets;
  p->strides = (STRIDE_TOTAL_TICKETS * 10) / p->tickets;

  // Return the number of tickets remaining for the current process after the transfer.
  tickets_transferred = p->tickets;

  return tickets_transferred;
}

