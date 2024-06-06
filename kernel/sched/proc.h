// Per-CPU state

struct cpu {
  uchar apicid;                // Local APIC ID
  struct context *scheduler;   // swtch() here to enter scheduler
  struct taskstate ts;         // Used by x86 to find stack for interrupt
  struct segdesc gdt[NSEGS];   // x86 global descriptor table
  volatile uint started;       // Has the CPU started?
  int ncli;                    // Depth of pushcli nesting.
  int intena;                  // Were trap enabled before pushcli?
  struct proc *proc;           // The process running on this cpu or null
};




extern struct cpu cpus[NCPU];
extern int ncpu;

//PAGEBREAK: 17
// Saved registers for kernel context switches.
// Don't need to save all the segment registers (%cs, etc),
// because they are constant across kernel contexts.
// Don't need to save %eax, %ecx, %edx, because the
// x86 convention is that the caller has saved them.
// Contexts are stored at the bottom of the stack they
// describe; the stack pointer is the address of the context.
// The layout of the context matches the layout of the stack in swtch.S
// at the "Switch stacks" comment. Switch doesn't save eip explicitly,
// but it is on the stack and allocproc() manipulates it.
struct context {
  uint edi;
  uint esi;
  uint ebx;
  uint ebp;
  uint eip;
};
/*
 * Added WAIT which will be a low priority sleep
 *
 * added PREEMPTED so we know a proc ran out of its time quantum
 */
enum procstate { UNUSED, EMBRYO, SLEEPING, RUNNABLE, RUNNING, ZOMBIE, WAIT, PREEMPTED};

#define TOP_PRIORITY             10
#define HIGH_KERNEL_PRIORITY     9
#define MED_KERNEL_PRIORITY      8 //default pri for kernel proc
#define LOW_KERNEL_PRIORITY      7
#define HIGH_USER_PRIORITY       5
#define MED_USER_PRIORITY        3  //default pri for user proc
#define LOW_USER_PRIORITY        1

#define DEFAULT_USER_TIME_QUANTUM    45//default user time quantum (clock cycles permitted for execution before pri drops and shceduler permits another proc to run)
#define DEFAULT_KERNEL_TIME_QUANTUM  100 // default kernel time quantum

#define URGENT                  1   //A way to suddenly indicate a process is high pri and just for this round
#define LOW                     2   //low pri just for this round

#define SCHED                  0    //is sched proc
#define KERNEL_PROC            1    // is kernel proc
#define USER_PROC              2    //is user proc

#define CHILD_SAME_PRI         0     //child of fork should maintain same pri
#define CHILD_DIFF_PRI         1    //child diff pri, decrement

#define ESIG                    1000000000    //Bad signal || no such signal
#define ENOPROC                 1000000001    // No proc of this pid found

//Important flags for PFLAG
#define IN_QUEUE               0x1
#define
// Per-process state
struct proc {
  uint sz;                     // Size of process memory (bytes)
  uint stack_base;              //stack base
  int p_sig;                   //The signal sent to this process
  void (*signal_handler)(int); // Pointer to signal handler function
  int p_ign;                   //flag to ignore signals (other than a kill, seg fault)
  char p_pri;                  // The priority of this process, for scheduling
  int p_time_quantum;          //The resident time for scheduling
  int p_cpu_usage;            //The amount of loops taken on this proc
  char p_flag;                 //Flag indicating many statuses of the proc
  int space_flag;              //flag to mark a process as either kernel space or user space
  int child_pri;               //A binary flag that will just indicate whether any children on fork should retain the same scheduling priority.
  pde_t* pgdir;                // Page table
  char *kstack;                // Bottom of kernel stack for this process
  enum procstate state;        // Process state
  int pid;                     // Process ID
  struct proc *parent;         // Parent process
  struct trapframe *tf;        // Trap frame for current syscall
  struct context *context;     // swtch() here to run process
  void *chan;                  // If non-zero, sleeping on chan
  int killed;                  // If non-zero, have been killed
  struct file *ofile[NOFILE];  // Open files
  struct inode *cwd;           // Current directory
  char name[16];               // Process name (debugging)
  struct proc *next;           // Will work this doubly linked list for scheduling right into the process table, like what was done with the buffer cache
  struct proc *prev;           // Will work this doubly linked list for scheduling right into the process table, like what was done with the buffer cache
};

// Process memory is laid out contiguously, low addresses first:
//   text
//   original data and bss
//   fixed-size stack
//   expandable heap

extern struct proctable {
    struct spinlock lock;
    struct proc proc[NPROC];
} ptable;


/* Dustyn's extra functions */
uint tally_allocated_memory_for_all_procs(void);
void inc_time_quantum(struct proc *p);
void change_process_space(int state_flag);
void preempt(void);
