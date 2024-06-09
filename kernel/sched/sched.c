//
// Created by dustyn on 6/5/24.
//

#include "../../user/types.h"
#include "../defs/defs.h"
#include "../lock/spinlock.h"
#include "../defs/param.h"
#include "../arch/x86_32/mem/memlayout.h"
#include "../arch/x86_32/mem/mmu.h"
#include "../arch/x86_32/x86.h"
#include "proc.h"
#include "../arch/x86_32/mem/vm.h"
#include "signals.h"
#include "sched.h"
#include "../data/queue.h"

/*
 * This will be a doubly linked list where processes of higher or lower priorities will be sorted either at the head or the ass end depending on priority.
 * This will help me avoid preempting lone processes because, of course, why would you preempt a lone process to spin when there is no process waiting.This will also
 * help with properly sorting processes depending on the priority factors as opposed to doing checks as you get to the process. My algorithm now works fine, but if there were lots of processes running, it
 * could lead to issues. This will help ensure fairness.
 *
 */


struct pqueue runqueue[NCPU];
struct pqueue sleepqueue;
struct pqueue readyqueue;

int queueinit = 0;


/*
 * After each proc has finished, we can add its cpu usage to the total_cpu_time
 * increment n_procs, and then divide total_cpu_time by n_procs to get our new mean
 */

struct cpu_avg{
    uint total_cpu_time;
    uint n_procs;
    uint avg;
};

struct cpu_avg c_avg;
//init our counter, dummy value to start with avg
void init_cpu_avg_counter(){
    c_avg.avg = 25;
    c_avg.n_procs = 0;
    c_avg.total_cpu_time = 0;
}


//Update with the new average
void update_cpu_avg(uint ticks){
    c_avg.total_cpu_time += ticks;
    c_avg.n_procs++;
    c_avg.avg = c_avg.total_cpu_time / c_avg.n_procs;
}


//get the avg clock cycles per proc
uint get_cpu_avg(){
    return c_avg.avg;
}


/*
 * Init our proc queue by linking the head and tail and initing the spinlock
 *
 *
 */

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
    int this_cpu = cpuid();

    for (;;) {
        // Enable trap on this processor.
        sti();


        // Loop over process table looking for process to run.

        main:
            if(readyqueue.head != 0){
               p = readyqueue.head;
            } else goto sched;
            //If there is an unhandled signal
            if(signals_pending(p)){
                handle_signals(p);
            }


            //prempted procs will need to go round the merry-go-round a few times before they are reset
            if (is_queue_empty(&runqueue[this_cpu]) && p->state == PREEMPTED && claim_proc(p,cpuid())) {
                p->state = RUNNABLE;
                //Add onto the time quantum with our averaged value, I prefer this to resetting the cpu_usage field because then
                //the calculations can get skewed
                p->p_time_quantum += c_avg.avg;
                insert_proc_into_queue(p,&runqueue[this_cpu]);
                goto sched;
            }
            if (p->state == PREEMPTED && p->p_pri != TOP_PRIORITY) {
                p->p_pri++;

            } else if (p->state == PREEMPTED && p->p_pri == TOP_PRIORITY) {
                p->state = RUNNABLE;
                p->p_pri = MED_USER_PRIORITY;
            }
            if (p->state != RUNNABLE) {
                continue;
            }
            if (claim_proc(p,this_cpu)) {
                insert_proc_into_queue(p,&runqueue[this_cpu]);
            }

            goto sched;

            sched:    // Switch to chosen process.  It is the process's job
            // to release ptable.lock and then reacquire it
            // before jumping back to us.
        acquire(&ptable.lock);
            if (is_queue_empty(&runqueue[this_cpu])) {
                goto main;
            }

            purge_queue(&runqueue[this_cpu]);
            c->proc = runqueue[this_cpu].head;
            switchuvm(runqueue[this_cpu].head);

            runqueue[this_cpu].head->state = RUNNING;

            swtch(&(c->scheduler), runqueue[this_cpu].head->context);
            switchkvm();
            shift_queue(&runqueue[this_cpu]);

            // Process is done running for now.
            // It should have changed its p->state before coming back.
            c->proc = 0;


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
    int cpu_id = cpuid();
    if (!holding(&ptable.lock))
        panic("sched ptable.lock");
    if (mycpu()->ncli != 1) {
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

    if(p->curr_cpu != NOCPU){
        remove_proc_from_queue(p,&runqueue[p->curr_cpu]);
    }
    //Put this process into the queue if it was not already there
    if (p->state == RUNNABLE && claim_proc(p,cpu_id)) {
       insert_proc_into_queue(p,&runqueue[cpu_id]);
    }

    swtch(&p->context, mycpu()->scheduler);
    mycpu()->intena = intena;

}

// Give up the CPU for one scheduling round.
void
yield(void) {
    acquire(&ptable.lock);  //DOC: yieldlock
    int this_cpu = cpuid();
    myproc()->state = RUNNABLE;
    purge_queue(&runqueue[this_cpu]);
    sched();
    release(&ptable.lock);
}

void preempt(void) {
    acquire(&ptable.lock);  //DOC: yieldlock
    int this_cpu = cpuid();
    myproc()->state = PREEMPTED;
    purge_queue(&runqueue[this_cpu]);
    sched();
    release(&ptable.lock);
}