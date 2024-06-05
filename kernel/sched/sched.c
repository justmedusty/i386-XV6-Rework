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
#include "signal.h"
#include "sched.h"

/*
 * This will be a doubly linked list where processes of higher or lower priorities will be sorted either at the head or the ass end depending on priority.
 * This will help me avoid preempting lone processes because, of course, why would you preempt a lone process to spin when there is no process waiting.This will also
 * help with properly sorting processes depending on the priority factors as opposed to doing checks as you get to the process. My algorithm now works fine, but if there were lots of processes running, it
 * could lead to issues. This will help ensure fairness.
 *
 */
struct pqueue procqueue;

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
int is_queue_empty() {
    int result = (procqueue.head == 0);
    return result;
}

int is_proc_alone_in_queue(struct proc *p){
    return (procqueue.head == p && procqueue.head->next == 0);
}

/*
 * This will traverse the queue , comparing priority, cpu usage against time quantum, and insert
 * the new process in an appropriate place in the queue. If there is nothing in the queue, it will be placed between head and tail.
 */
void insert_proc_into_queue(struct proc *new){
    acquire(&procqueue.qloc);

    if (procqueue.head == 0) {

        procqueue.head = new;
        new->next = 0;
        new->prev = 0;
        release(&procqueue.qloc);
        return;

    }
    if (procqueue.head->next == 0) {
        procqueue.tail = new;
        new->next = 0;
        new->prev = procqueue.head;
        procqueue.head->next = new;
        release(&procqueue.qloc);
        return;
    }

    for (struct proc *this = procqueue.head->next; this->next != 0; this = this->next) {
        if (this->state == RUNNABLE && (new->p_pri > this->p_pri || new->p_flag == URGENT)) {
            // Update pointers for the new process
            new->prev = this->prev;
            new->next = this;

            if (this->prev != 0) {
                this->prev->next = new;
            } else {
                procqueue.head = new;
            }

            this->prev = new;

            release(&procqueue.qloc);
            return;
        }
    }

    procqueue.tail->next = new;
    new->prev = procqueue.tail;
    new->next = 0;
    procqueue.tail = new;
    release(&procqueue.qloc);
    return;

}

/*
 * Check if proc is queued
 */
int is_proc_queued(struct proc *p) {

    struct proc *pointer = procqueue.head;
    while (pointer != 0) {
        if (p == pointer) {
            return 1;
        }
        pointer = pointer->next;
    }
    return 0;
}


/*
 * Remove this process from the queue
 */
void remove_proc_from_queue(struct proc *old) {

    acquire(&procqueue.qloc);

// Handle the case when the process to remove is at the head of the queue
    if (procqueue.head == old) {
        procqueue.head = old->next;
        if (procqueue.head) {
            procqueue.head->prev = 0;
        } else {
            procqueue.tail = 0; // Queue becomes empty
        }

        release(&procqueue.qloc);
        return;
    }

// Loop through the queue to find the process to remove
    for (struct proc *this = procqueue.head; this != 0; this = this->next) {
        if (this == old) {
            // Update pointers to remove the process
            if (this->prev) {
                this->prev->next = this->next;
            }
            if (this->next) {
                this->next->prev = this->prev;
            } else {
                procqueue.tail = this->prev; // Update tail if the process is at the tail
            }

            release(&procqueue.qloc);
            return;
        }
    }
    if (procqueue.head == 0) {
        release(&procqueue.qloc);
        panic("proc not in queue");
    }
}

/*
 * Purge all invalid states from the run queue. Only runnable procs should be in the queue.
 */
void purge_queue() {

    struct proc *pointer = procqueue.head;

    while (pointer != 0) {

        if (pointer->state != RUNNABLE) {

            remove_proc_from_queue(pointer);

        }

        pointer = pointer->next;
    }

}

/*
 * This shifts the head to the tail , for after a scheduling round has completed
 */
void shift_queue() {

    acquire(&procqueue.qloc);

    if (procqueue.head == 0 || procqueue.head->next == 0) {
        release(&procqueue.qloc);
        return;
    }

    struct proc *old_head = procqueue.head;
    struct proc *new_head = procqueue.head->next;


    new_head->prev = 0;
    procqueue.head = new_head;
    old_head->next = 0;
    old_head->prev = procqueue.tail;
    procqueue.tail->next = old_head;
    procqueue.tail = old_head;

    release(&procqueue.qloc);

    if (procqueue.head != 0 && procqueue.head == procqueue.tail) {
        panic("head eq tail");
    }

}
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
    main:
    for (;;) {
        // Enable trap on this processor.
        sti();


        // Loop over process table looking for process to run.
        acquire(&ptable.lock);
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
            if (is_queue_empty() && p->state == PREEMPTED && !is_proc_queued(p)) {
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

            if (!is_proc_queued(p)) {
                insert_proc_into_queue(p);
            }


            goto sched;

            sched:    // Switch to chosen process.  It is the process's job
            // to release ptable.lock and then reacquire it
            // before jumping back to us.

            if (is_queue_empty()) {
                goto main;
            }


            shift_queue();
            c->proc = procqueue.head;
            switchuvm(procqueue.head);
            procqueue.head->state = RUNNING;

            swtch(&(c->scheduler), procqueue.head->context);
            switchkvm();

            purge_queue();


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
    //Put this process into the queue if it was not already there
    if (p->state == RUNNABLE && !is_proc_queued(p)) {
        insert_proc_into_queue(p);
    }

    swtch(&p->context, mycpu()->scheduler);
    mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void) {
    acquire(&ptable.lock);  //DOC: yieldlock
    myproc()->state = RUNNABLE;
    purge_queue();
    sched();
    release(&ptable.lock);
}

void preempt(void) {
    acquire(&ptable.lock);  //DOC: yieldlock
    myproc()->state = PREEMPTED;
    purge_queue();
    sched();
    release(&ptable.lock);
}