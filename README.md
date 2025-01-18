# XV6 Operating System Projects

This repository contains three projects focused on extending and modifying the xv6 operating system, demonstrating various OS concepts including system calls, process scheduling, and memory management.

## Project 1: System Call Monitoring
### Description
Implemented functionality to count and track system call invocations in xv6, focusing on:
- Counting invocations of `fork()`, `wait()`, and `exit()` system calls
- Creating new system calls for monitoring purposes
- Implementing user-space interfaces for system call tracking

### Key Features
- System call invocation counter
- Reset functionality for counters
- User-space wrapper functions for counter access

## Project 2: Race Conditions & Stride Scheduling
### Description
Enhanced xv6's process scheduling system with two major components:

#### Part 1: Fork Race Condition Management
- Implemented alternating fork winner feature
- Created mechanism for parent/child process execution order control
- Added system calls for controlling race condition behavior

#### Part 2: Stride Scheduler Implementation
- Implemented a stride scheduling algorithm
- Added ticket-based process priority system
- Created ticket transfer functionality between processes
- Maintained total system ticket count of 100

### Key Features
- Configurable scheduling policy (Round Robin vs Stride)
- Ticket ownership tracking
- Process priority management
- Fair scheduling based on ticket distribution

## Project 3: Shared Memory Implementation
### Description
Added shared memory functionality to xv6, enabling inter-process communication through:

- Static Mapped Shared Memory Page (SMSMP)
- Dynamic Mapped Shared Memory Page (DMSMP)
- Memory management system modifications

### Key Features
- Two types of shared memory pages
- Dynamic memory allocation on first write
- Memory reference counting
- Process-safe memory sharing

## Technical Details

### Environment
- XV6 Operating System
- QEMU Emulator
- C Programming Language

### Core Components Modified
- Process Management System
- Memory Management Unit
- System Call Interface
- Scheduling Subsystem

### Testing
Each project includes comprehensive test cases verifying:
- Functionality correctness
- Edge case handling
- Performance metrics
- System stability

## Building and Running

1. Clone the repository
2. Enter project directory
3. Build XV6:
```bash
make clean
make qemu-nox
```

## Project Structure
```
.
├── proj1-syscalls/        # System call monitoring implementation
├── proj2-scheduling/      # Race condition and stride scheduler
└── proj3-shared-memory/   # Shared memory implementation
```

## Key Learnings
- Operating system internals
- System call implementation
- Process scheduling algorithms
- Memory management techniques
- Race condition handling
- Inter-process communication

## Author
Narendra Khatpe
- Master's in Computer Science, SUNY Binghamton
- Contact: narendrakhatpe@gmail.com
