#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"
#include "vm.h"
#include "signal.h"

struct {
    struct spinlock lock;
    struct proc proc[NPROC];
} ptable;


/*
 * This will be a doubly linked list where processes of higher or lower priorities will be sorted either at the head or the ass end depending on priority.
 * This will help me avoid preempting lone processes because, of course, why would you preempt a lone process to spin when there is no process waiting.This will also
 * help with properly sorting processes depending on the priority factors as opposed to doing checks as you get to the process. My algorithm now works fine, but if there were lots of processes running, it
 * could lead to issues. This will help ensure fairness.
 *
 */
struct {
    struct spinlock qloc;
    struct proc *head;
    struct proc *tail;
} procqueue;

int queueinit = 0;

/*
 * I think I will do live calculations of average cpu usage in clock cycles, update this obj-wide variable and let
 * that be a basis for preempting..
 *
 * I'll start it with a dummy value
 *
 * todo implement live calculation of avg cpu usage
 */
static int avg_cpu_usage = 25;

static struct proc *initproc;


int nextpid = 1;

extern void forkret(void);

extern void trapret(void);

static void wakeup1(void *chan);

void
pinit(void) {
    initlock(&ptable.lock, "ptable");
}

int is_queue_empty(){
    int result = (procqueue.head == 0);
    return result;
}


/*
 * This will traverse the queue , comparing priority, cpu usage against time quantum, and insert
 * the new process in an appropriate place in the queue. If there is nothing in the queue, it will be placed between head and tail.
 */
void insert_proc_into_queue(struct proc *new) {
    acquire(&procqueue.qloc);

    if (procqueue.head == 0 ) {

        procqueue.head = new;
        new->next = 0;
        new->prev = 0;
        release(&procqueue.qloc);
        return;

    }
    if(procqueue.head->next == 0){
        procqueue.tail = new;
        new->next = 0;
        new->prev = procqueue.head;
        release(&procqueue.qloc);
        return;
    }

    for (struct proc *this = procqueue.head->next; this->next != 0; this = this->next) {

        if (this->state == RUNNABLE && (new->p_pri > this->p_pri || new->p_flag == URGENT || this->p_cpu_usage < avg_cpu_usage)) {

            this->next = new->next;
            this->prev = new->prev;
            new->prev->next = new;
            new->next->prev = new;
            release(&procqueue.qloc);
            return;

        }
    }


}

/*
 * Remove this process from the queue
 */
 void remove_proc_from_queue(struct proc *old) {

    acquire(&procqueue.qloc);

    for (struct proc *this = procqueue.head; this != 0; this = this->next) {

        if (this == old) {

            this->next->prev = this->prev;
            this->prev->next = this->next;
            release(&procqueue.qloc);
            return;

        }

    }
    if (procqueue.head == 0) {
        release(&procqueue.qloc);
        panic("proc not in queue");

    }
}

// Must be called with interrupts disabled
int
cpuid() {
    return mycpu() - cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu *
mycpu(void) {
    int apicid, i;

    if (readeflags() & FL_IF)
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

/*
 * This will iterate through each process and tally up allocated pages in each processes page directory as
 * well as tallying up allocated kernel pages.
 */
int freemem(void) {
    uint total_pages = 0;
    struct proc *p;
    // Tally memory used by all procs

    acquire(&ptable.lock);


    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
        if (p->state != UNUSED) {
            total_pages += tally_page_directory(p->pgdir);
        }

    }
    release(&ptable.lock);

    return total_pages;
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc *
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
static struct proc *
allocproc(void) {
    struct proc *p;
    char *sp;

    acquire(&ptable.lock);


    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
        if (p->state == UNUSED)
            goto found;

    release(&ptable.lock);
    return 0;

    found:
    p->state = EMBRYO;
    p->pid = nextpid++;


    release(&ptable.lock);

    // Allocate kernel stack.
    if ((p->kstack = kalloc()) == 0) {
        p->state = UNUSED;
        return 0;
    }
    sp = p->kstack + KSTACKSIZE;


    // Leave room for trap frame.
    sp -= sizeof *p->tf;
    p->tf = (struct trapframe *) sp;

    // Set up new context to start executing at forkret,
    // which returns to trapret.
    sp -= 4;
    *(uint *) sp = (uint) trapret;

    sp -= sizeof *p->context;
    p->context = (struct context *) sp;
    memset(p->context, 0, sizeof *p->context);
    p->context->eip = (uint) forkret;

    return p;
}

/*
 * Init our proc queue by linking the head and tail and initing the spinlock
 *
 *
 */
void initprocqueue() {

    initlock(&procqueue.qloc, "procqueue");
    procqueue.head = 0;
    procqueue.tail = 0;


}


//PAGEBREAK: 32
// Set up first user process.
void
userinit(void) {
    initprocqueue();
    struct proc *p;
    extern char _binary_initcode_start[], _binary_initcode_size[];

    p = allocproc();

    initproc = p;
    if ((p->pgdir = setupkvm()) == 0)
        panic("userinit: out of memory?");
    inituvm(p->pgdir, _binary_initcode_start, (int) _binary_initcode_size);
    p->sz = PGSIZE;
    memset(p->tf, 0, sizeof(*p->tf));
    p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
    p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
    p->tf->es = p->tf->ds;
    p->tf->ss = p->tf->ds;
    p->tf->eflags = FL_IF;
    p->tf->esp = PGSIZE;
    p->tf->eip = 0;  // beginning of initcode.S
    p->space_flag = USER_PROC;

    /*
     * Setting our new scheduling fields
     */
    p->p_time_quantum = DEFAULT_USER_TIME_QUANTUM;
    p->child_pri = CHILD_SAME_PRI;
    p->p_pri = MED_USER_PRIORITY;
    p->space_flag = USER_PROC;

    /*
     * Setting up intr/ signal fields
     */

    p->signal_handler = NULL;
    //ignore no signals
    p->p_ign = 0;
    //start without any signals obviously
    p->p_sig = 0;


    safestrcpy(p->name, "initcode", sizeof(p->name));
    //The init process' current working directory will be the root dir inode
    p->cwd = namei("/");

    // this assignment to p->state lets other cores
    // run this process. the acquire forces the above
    // writes to be visible, and the lock is also needed
    // because the assignment might not be atomic.
    acquire(&ptable.lock);

    p->state = RUNNABLE;


    release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n) {
    uint sz;
    struct proc *curproc = myproc();

    sz = curproc->sz;
    if (n > 0) {
        if ((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
            return -1;
    } else if (n < 0) {
        if ((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
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
fork(void) {
    int i, pid;
    struct proc *np;
    struct proc *curproc = myproc();

    // Allocate process.
    if ((np = allocproc()) == 0) {
        return -1;
    }

    // Copy process state from proc.
    if ((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0) {
        kfree(np->kstack);
        np->kstack = 0;
        np->state = UNUSED;
        return -1;
    }
    np->sz = curproc->sz;
    np->parent = curproc;
    *np->tf = *curproc->tf;
    /*
     * We will see the approriate scheduling flags and whatnot , we will check
     * the child pri flag to indicate whether or not the scheduler should
     */
    np->p_flag = curproc->p_flag;

    if (curproc->child_pri == CHILD_SAME_PRI) {

        np->p_pri = curproc->p_pri;

    } else {

        int new_pri = (curproc->p_pri == 0) ? LOW_USER_PRIORITY : (curproc->p_pri - 1);

        np->p_pri = new_pri;

    }
    /*
     * Clear time taken and copy the time quantum from the parent
     */
    np->p_cpu_usage = 0;
    np->p_time_quantum = curproc->p_time_quantum;

    /*
     * Fill the intr/ign/handler fields
     */


    //null pointer for handler
    np->signal_handler = (void *) 0;
    //non-zero, do not ignore signals
    np->p_ign = 0;
    //we don't want a proc to have a signal at birth
    np->p_sig = 0;


    // Clear %eax so that fork returns 0 in the child.


    for (i = 0; i < NOFILE; i++)
        if (curproc->ofile[i])
            np->ofile[i] = filedup(curproc->ofile[i]);
    np->cwd = idup(curproc->cwd);

    safestrcpy(np->name, curproc->name, sizeof(curproc->name));
    np->tf->eax = 0;
    pid = np->pid;
    cprintf("ppid is %d\n",np->parent->pid);
    cprintf("pid is %d\n",np->pid);

    acquire(&ptable.lock);

    np->state = RUNNABLE;

    release(&ptable.lock);


    return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.

/*
 * We will get rid of the zombieification of processes. When a child exits there will no longer be a zombie that is required
 * to be reaped, but instead we will clear the process entry in the process table and zero it.
 * Dustyn - 5/7/24
 */
void
exit(void) {
    struct proc *curproc = myproc();
    struct proc *p;
    int fd;

    if (curproc == initproc) {
        panic("initproc exiting");
    }


    // Close all open files.
    for (fd = 0; fd < NOFILE; fd++) {
        if (curproc->ofile[fd]) {
            fileclose(curproc->ofile[fd]);
            curproc->ofile[fd] = 0;
        }
    }

    begin_op();
    iput(curproc->cwd);
    end_op();
    curproc->cwd = 0;

    acquire(&ptable.lock);

    // Parent might be sleeping in wait().
    wakeup1(curproc->parent);

    // Pass abandoned children to init.
    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
        if (p->parent == curproc) {
            p->parent = initproc;
            wakeup1(initproc);
        }
    }
    remove_proc_from_queue(curproc);
    // curproc->name[0] = 0;
    // curproc->killed = 0;
    curproc->state = ZOMBIE;
    curproc->killed = 1;
    nextpid = curproc->pid;
    //freevm(curproc->pgdir);
    //kfree(curproc->kstack);

    // Jump into the scheduler, never to return.
    sched();
    panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void) {
    struct proc *p;
    int havekids, pid;
    struct proc *curproc = myproc();

    acquire(&ptable.lock);
    for (;;) {
        // Scan through table looking for exited children.
        havekids = 0;
        for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
            if (p->parent != curproc)
                continue;
            havekids = 1;
            if (p->state == ZOMBIE) {
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
        if (!havekids || curproc->killed) {
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
scheduler(void) {
    struct proc *p;
    struct cpu *c = mycpu();
    //Init to a null pointer so we can assign the first proc to it and then from there keep checking.
    c->proc = 0;

    for (;;) {
        // Enable interrupts on this processor.
        sti();


        // Loop over process table looking for process to run.
        acquire(&ptable.lock);
        main:
        for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {

            //If there is an unhandled signal
            if (p->p_sig != 0) {

                /*
                 * If the signal is one of the fatal signals, terminate no matter what
                 * and print to console that the process received a fatal signal
                 */
                if (((p->p_sig & SIGKILL) != 0) || ((p->p_sig & SIGSEG) != 0) || ((p->p_sig & SIGPIPE) != 0)) {
                    cprintf("pid %d received fatal signal\n", p->pid);
                    p->killed = 1;
                }
                /*
                 * If the time quantum has been exceeded, goto sched and let another process run.
                 * This is handled here with kill seg pipe etc because it cannot be ignored.
                 */
                if ((p->p_sig & SIGCPU) != 0) {
                    p->p_sig &= ~SIGCPU;
                    release(&ptable.lock);
                    yield();
                }

                /*
                 * If there no ignore mask, terminate
                 */
                if (p->p_ign == 0) {

                    p->killed = 1;
                    /*
                     * If the ignore is masking the non fatal signal, ignore it
                     */
                } else if ((p->p_ign & p->p_sig) != 0) {

                    p->p_sig = 0;

                    /*
                     * else if the signals being ignored do not cover the signal received, terminate the process.
                     */
                } else {
                    p->killed = 1;

                }

            }

            //prempted procs will need to go round the merry-go-round a few times before they are reset
            if(is_queue_empty() && p->state == PREEMPTED){
                p->state = RUNNABLE;
                insert_proc_into_queue(p);
                goto sched;
            }
            if (p->state == PREEMPTED && p->p_pri != TOP_PRIORITY) {
                p->p_pri++;
                //We will reset the cpu usage when a process is lifted out of preempted state,
                //this way we can start to implement logic into our algorithim that will
                //take recent cpu usage into account. Otherwise, cpu usage would rise forever, and you would
                //not know if it was recent..
            } else if (p->state == PREEMPTED && p->p_pri == TOP_PRIORITY) {
                p->state = RUNNABLE;
                p->p_pri = MED_USER_PRIORITY;
            }
            if (p->state != RUNNABLE) {
                continue;
            }

            insert_proc_into_queue(p);


            goto sched;

            sched:    // Switch to chosen process.  It is the process's job
            // to release ptable.lock and then reacquire it
            // before jumping back to us.


            if(is_queue_empty()){
                goto main;
            }

            c->proc = procqueue.head;
            switchuvm(procqueue.head);
            procqueue.head->state = RUNNING;



            swtch(&(c->scheduler), procqueue.head->context);
            switchkvm();

            // Process is done running for now.
            // It should have changed its p->state before coming back.
            c->proc = 0;

        }
        release(&ptable.lock);

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
sched(void) {
    int intena;
    struct proc *p = myproc();
    if (!holding(&ptable.lock))
        panic("sched ptable.lock");
    if (mycpu()->ncli != 1){
        panic("sched locks");
    }
    if (p->state == RUNNING)
        panic("sched running");
    if (readeflags() & FL_IF)
        panic("sched interruptible");
    intena = mycpu()->intena;

    //Is this a higher priority than the current process? if so, set it to URGENT so that it will be swapped in immediately
    if (mycpu()->proc->p_pri < p->p_pri || mycpu()->proc->space_flag < p->space_flag) {
        p->p_flag = URGENT;
    }
    //Put this process into the scheduler
    insert_proc_into_queue(p);
    swtch(&p->context, mycpu()->scheduler);
    mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void) {
    acquire(&ptable.lock);  //DOC: yieldlock
    myproc()->state = RUNNABLE;

    sched();
    release(&ptable.lock);
}

void preempt(void) {
    acquire(&ptable.lock);  //DOC: yieldlock
    myproc()->state = PREEMPTED;
    sched();
    release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void) {
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
sleep(void *chan, struct spinlock *lk) {
    struct proc *p = myproc();

    if (p == 0)
        panic("sleep");

    if (lk == 0)
        panic("sleep without lk");

    // Must acquire ptable.lock in order to
    // change p->state and then call sched.
    // Once we hold ptable.lock, we can be
    // guaranteed that we won't miss any wakeup
    // (wakeup runs with ptable.lock locked),
    // so it's okay to release lk.
    if (lk != &ptable.lock) {  //DOC: sleeplock0
        acquire(&ptable.lock);  //DOC: sleeplock1
        release(lk);
    }
    // Go to sleep.
    p->chan = chan;
    remove_proc_from_queue(p);
    p->state = SLEEPING;


    sched();
    // Tidy up.
    p->chan = 0;

    // Reacquire original lock.
    if (lk != &ptable.lock) {  //DOC: sleeplock2
        release(&ptable.lock);
        acquire(lk);

    }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan) {
    struct proc *p;

    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
        if (p->state == SLEEPING && p->chan == chan) {
            p->state = RUNNABLE;
            p->p_flag = URGENT;
            p->p_pri++;
        }
}


/*
 * This is for waking up higher priority processes
 *
 * unimplemented right now
 */
static void
wakeup2(void *chan) {
    struct proc *p;

    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
        if (p->state == WAIT && p->chan == chan)
            p->state = RUNNABLE;
}


// Wake up all processes sleeping on chan.
void
wakeup(void *chan) {
    acquire(&ptable.lock);
    wakeup1(chan);
    release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid) {
    struct proc *p;

    acquire(&ptable.lock);
    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
        if (p->pid == pid) {
            p->killed = 1;
            // Wake process from sleep if necessary.
            if (p->state == SLEEPING)
                p->state = RUNNABLE;
            release(&ptable.lock);
            return 0;
        }
    }
    release(&ptable.lock);
    return -1;
}


/*
 * Send a signal to a process, we will do a check to ensure it's a valid signal.
 * We will loop through until we find the pid and change its signal and then
 * change its priority to high and reset its time quantum so that it will be scheduled quickly.
 *
 * We will also change the state to runnable , making the scheduler pick it up and process the signal quickly.
 */
int sig(int sigmask, int pid) {

    struct proc *proc;
    //make sure this is a valid signal
    if (sigmask > (SIGKILL + SIGSEG + SIGINT + SIGPIPE + SIGHUP + SIGSYS + SIGCPU)) {
        return ESIG;
    }


    acquire(&ptable.lock);
    for (proc = ptable.proc; proc < &ptable.proc[NPROC]; proc++) {
        if (proc->pid == pid && proc->state != UNUSED) {
            //set the signal
            proc->p_sig |= sigmask;
            // Wake process from sleep if necessary.
            if (proc->state != RUNNING) {
                proc->state = RUNNABLE;
            }
            proc->p_pri = TOP_PRIORITY;
            release(&ptable.lock);
            return 0;
        }


    }
    release(&ptable.lock);
    return ENOPROC;
}

/*
 * This will be a system call for setting a processes signal
 *
 * This does not work yet I will need to find a way to stash the program counter / eip at the right spot and have it be accessible in kernel memory.
 */
void sighandler(void (*func)(int)) {
    acquire(&ptable.lock);
    myproc()->signal_handler = V2P(func);
    cprintf("func = %d\n", func);
    release(&ptable.lock);
    return;

}

/*

 * This system call let's a process ignore non fatal signals
 * sigmask is the bitmask containing which signals the userspace
 * program is lookling to either mask or enable
 *
 * If 0, unmask specified signal bits
 *
 * if 1, mask specified signals when they come in
 *
 */
void sigignore(int sigmask, int action) {
    struct proc *p = myproc();
    acquire(&ptable.lock);
    if (action == 0) {
        p->p_ign &= sigmask;
    } else {
        //enable the specified signal bits
        p->p_ign |= sigmask;
        cprintf("PID %d has turned some signal interrupts off\n", p->pid);
    }
    release(&ptable.lock);
    return;

}


//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void) {
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

    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
        if (p->state == UNUSED)
            continue;
        if (p->state >= 0 && p->state < NELEM(states) && states[p->state])
            state = states[p->state];
        else
            state = "???";
        cprintf("%d %s %s", p->pid, state, p->name);
        if (p->state == SLEEPING) {
            getcallerpcs((uint *) p->context->ebp + 2, pc);
            for (i = 0; i < 10 && pc[i] != 0; i++)
                cprintf(" %p", pc[i]);
        }
        cprintf("\n");
    }
}


/*
 * Changes the space flag in the process table to indicate this process is running in ring 0 , probably from a system
 * call context switch
 */
void change_process_space(int state_flag) {
    if (state_flag == 1) {
        myproc()->space_flag = KERNEL_PROC;
    } else {
        myproc()->space_flag = USER_PROC;
    }
    return;
}