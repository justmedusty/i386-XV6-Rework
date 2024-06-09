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

struct cpu_avg {
    uint total_cpu_time;
    uint n_procs;
    uint avg;
};

struct cpu_avg c_avg;

//init our counter, dummy value to start with avg
void init_cpu_avg_counter() {
    c_avg.avg = 25;
    c_avg.n_procs = 0;
    c_avg.total_cpu_time = 0;
}


//Update with the new average
void update_cpu_avg(uint ticks) {
    c_avg.total_cpu_time += ticks;
    c_avg.n_procs++;
    c_avg.avg = c_avg.total_cpu_time / c_avg.n_procs;
}


//get the avg clock cycles per proc
uint get_cpu_avg() {
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
    unsigned char qmask;
    //Init to a null pointer so we can assign the first proc to it and then from there keep checking.
    c->proc = 0;
    int this_cpu = cpuid();

    for (;;) {
        // Enable trap on this processor.
        sti();
        main:
       //tight loop, can fill this with other tasks and routines later
        if (is_queue_empty(&readyqueue) && is_queue_empty(&runqueue[this_cpu])){
            //Check if the queues are balanced, if not pluck some out and place them in the ready queue where this cpu can snag them.
            if((qmask = queues_need_balance()) > 0 ){
                do_balance(qmask);
            }
            goto main;
        }
        if (!is_queue_empty(&readyqueue)) {
            p = readyqueue.head;
        } else{
            goto sched;
        }

        //If there is an unhandled signal
        if (signals_pending(p)) {
            handle_signals(p);
        }

        if (p->curr == &readyqueue && claim_proc(p, this_cpu)) {
            remove_proc_from_queue(p,&readyqueue);
            insert_proc_into_queue(p, &runqueue[this_cpu]);
        }
        goto sched;

        sched:

        if (is_queue_empty(&runqueue[this_cpu])) {
           goto main;
        }
        c->proc = runqueue[this_cpu].head;
        switchuvm(runqueue[this_cpu].head);
        acquire(&ptable.lock);
        runqueue[this_cpu].head->state = RUNNING;


        swtch(&(c->scheduler), runqueue[this_cpu].head->context);
        shift_queue(&runqueue[this_cpu]);
        switchkvm();
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

    if (p->curr_cpu != NOCPU) {
        unclaim_proc(p);
        remove_proc_from_queue(p, &runqueue[p->curr_cpu]);
    }

    if(p->state == PREEMPTED){
        p->p_pri = LOW_USER_PRIORITY;
        p->state = RUNNABLE;
        p->curr = 0;
        insert_proc_into_queue(p,&readyqueue);
        goto done;
    }

    //Put this process into the queue if it was not already there
    if (p->state == RUNNABLE && p->curr != 0) {
        remove_proc_from_queue(p, p->curr);
        insert_proc_into_queue(p, &readyqueue);
    } else if (p->state == RUNNABLE){
        insert_proc_into_queue(p, &readyqueue);
    }

    done:
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
    struct proc *p =  myproc();
    int this_cpu = cpuid();
    p->state = PREEMPTED;
    purge_queue(&runqueue[this_cpu]);
    sched();
    release(&ptable.lock);
}