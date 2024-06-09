//
// Created by dustyn on 6/5/24->
//
#include "../../user/types.h"
#include "../lock/spinlock.h"
#include "queue.h"
#include "../../user/types.h"
#include "../defs/defs.h"
#include "../defs/param.h"
#include "../arch/x86_32/mem/memlayout.h"
#include "../arch/x86_32/mem/mmu.h"
#include "../arch/x86_32/x86.h"
#include "../sched/proc.h"
#include "../arch/x86_32/mem/vm.h"
#include "../sched/signals.h"
#include "../arch/x86_32/mp/mp.h"
#include "../sched/sched.h"

/*
 * We will create some reusable functions for dealing with proc queues and other types of queues in order to make it simpler to deal with many different
 * data structures, can reuse the wrapper functions defined here
 */

//this lock is just for a cpu claiming a process
struct spinlock check_lock;
int lock_init = 0;
//*************************************************************
//PROC QUEUES
//*************************************************************

void initprocqueue(struct pqueue *procqueue) {
    if(!lock_init){
        initlock(&check_lock,"claimlock");
    }
    initlock(&procqueue->qloc, "procqueue");
    procqueue->head = 0;
    procqueue->tail = 0;
    procqueue->len = 0;
}

int is_queue_empty(struct pqueue *procqueue) {
    int result = (procqueue->head == 0);
    return result;
}
int is_proc_alone_in_queue(struct proc *p,struct pqueue *procqueue){
    return (procqueue->head == p && procqueue->head->next == 0);
}

/*
 * This will traverse the queue , comparing priority, cpu usage against time quantum, and insert
 * the new process in an appropriate place in the queue-> If there is nothing in the queue, it will be placed between head and tail->
 */
void insert_proc_into_queue(struct proc *new,struct pqueue *procqueue){

    if(new->state == RUNNING){
        panic("Inserting running proc");
    }

    acquire(&procqueue->qloc);
    if (procqueue->head == 0) {

        procqueue->head = new;
        new->next = 0;
        new->prev = 0;
        procqueue->len++;
        new->curr = procqueue;
        release(&procqueue->qloc);
        return;

    }
    if (procqueue->head->next == 0) {
        procqueue->tail = new;
        new->next = 0;
        new->prev = procqueue->head;
        procqueue->head->next = new;
        procqueue->len++;
        new->curr = procqueue;
        release(&procqueue->qloc);
        return;
    }

    for (struct proc *this = procqueue->head->next; this->next != 0; this = this->next) {
        if (this->state == RUNNABLE && (new->p_pri > this->p_pri || new->p_flag == URGENT)) {
            // Update pointers for the new process
            new->prev = this->prev;
            new->next = this;

            if (this->prev != 0) {
                this->prev->next = new;
            } else {
                procqueue->head = new;
            }

            this->prev = new;
            procqueue->len++;
            new->curr = procqueue;
            release(&procqueue->qloc);
            return;
        }
    }

    procqueue->tail->next = new;
    new->prev = procqueue->tail;
    new->next = 0;
    procqueue->tail = new;
    procqueue->len++;
    new->curr = procqueue;
    release(&procqueue->qloc);
    return;

}

/*
 * Check if proc is queued, mark it taken if so
 * The nature of this function means it needs to be checked at the end of a check ie not
 * if claim_proc && something else, it needs to be something && something else && claim_proc
 *
 *
 *   *   *   *   *   *   *   *   *   *   *   *   *   *   *   *   *   *   *   *   *   *
 * YOU MUST QUEUE AFTER A CLAIM OTHERWISE IT WILL NOT BE ACCESSIBLE TO ANY RUNQUEUE!  *
 *   *   *   *   *   *   *   *   *   *   *   *   *   *   *   *   *   *   *   *   *   *
 */
int claim_proc(struct proc *p,int cpu) {

    acquire(&check_lock);

    int result = 0;
   if((p->queue_mask & IN_QUEUE) == 0){
       p->queue_mask |= IN_QUEUE;
       result = 1;
       p->curr_cpu = cpu;
   }
    release(&check_lock);
    return result;
}
//unclaim proc when dequeueing it
int unclaim_proc(struct proc *p) {
    acquire(&check_lock);
    int result = 0;
    if(p->queue_mask & IN_QUEUE){
        p->queue_mask &= ~IN_QUEUE;
        result = 1;
        p->curr_cpu = NOCPU;
        p->curr = 0;
    }
    release(&check_lock);
    return result;
}

/*
 * Remove this process from the queue
 */
void remove_proc_from_queue(struct proc *old,struct pqueue *procqueue) {

    if(old->state == RUNNING){
        panic("Removing running proc");
    }

    acquire(&procqueue->qloc);

// Handle the case when the process to remove is at the head of the queue
    if (procqueue->head == old) {
        procqueue->head = old->next;
        if (procqueue->head) {
            procqueue->head->prev = 0;
        } else {
            procqueue->tail = 0; // Queue becomes empty
        }
        procqueue->len--;
        unclaim_proc(old);
        release(&procqueue->qloc);
        return;
    }

// Loop through the queue to find the process to remove
    for (struct proc *this = procqueue->head; this != 0; this = this->next) {
        if (this == old) {
            // Update pointers to remove the process
            if (this->prev) {
                this->prev->next = this->next;
            }
            if (this->next) {
                this->next->prev = this->prev;
            } else {
                procqueue->tail = this->prev; // Update tail if the process is at the tail
            }
            procqueue->len--;
            unclaim_proc(old);
            release(&procqueue->qloc);
            return;
        }
    }
    if (procqueue->head == 0) {
        procqueue->len--;
        unclaim_proc(old);
        release(&procqueue->qloc);
        panic("proc not in queue");
    }
}

/*
 * Purge all invalid states from the run queue-> Only runnable procs should be in the queue->
 */
void purge_queue(struct pqueue *procqueue) {
    acquire(&procqueue->qloc);
    struct proc *pointer = procqueue->head;

    while (pointer != 0) {

        if (pointer->state != RUNNABLE) {
            release(&procqueue->qloc);
            remove_proc_from_queue(pointer,procqueue);
            acquire(&procqueue->qloc);
        }

        pointer = pointer->next;
    }
    release(&procqueue->qloc);
}

/*
 * This plucks the head out of the queue and allows it to be picked up by another cpu 
 */
void shift_queue(struct pqueue *procqueue) {
    if(procqueue->head && procqueue->head->state == RUNNING){
        panic("Removing running proc");
    }

    acquire(&procqueue->qloc);

    if (procqueue->head == 0 || procqueue->head->next == 0) {
        release(&procqueue->qloc);
        return;
    }

    struct proc *old_head = procqueue->head;
    struct proc *new_head = procqueue->head->next;


    new_head->prev = 0;
    procqueue->head = new_head;
    old_head->next = 0;
    unclaim_proc(old_head);
    release(&procqueue->qloc);

    if (procqueue->head != 0 && procqueue->head == procqueue->tail) {
        panic("head eq tail");
    }

}

// check the per-cpu rqs to see if we need to rebalance the queues
void queues_need_balance(){

    int ncpu = num_cpus();
    int tasks_per_rq[ncpu];
    struct proc *pointer;

    for (int i = 0; i < ncpu; i++) {
        tasks_per_rq[i] = runqueue[i].len;
    }

    int ideal_queue_len = 0;

    for (int i = 0; i < ncpu ; i++) {
        ideal_queue_len += tasks_per_rq[i];
    }

    ideal_queue_len /= ncpu;
    //25% grace cause we don't need to be exact just close enough
    int padded_ideal = (ideal_queue_len * 125) / 100;

    unsigned char unbalanced_rq_mask = 0;

    for (int i = 0; i < ncpu; ++i) {

        if(runqueue[i].len > padded_ideal){

            unbalanced_rq_mask |= (1 << i);

        }
    }

    return unbalanced_rq_mask;
}

//balance the queues, you need to pass the rq mask returned from queues_need_balance to save some computation
void do_balance(unsigned char rq_mask){

    int ncpu = num_cpus();
    unsigned char inverted_run_mask = ~rq_mask;
    for (int i = 0; i < ncpu; i++) {
        if(rq_mask & (1 << i)){

        }
    }

}
//************************************************
//OTHER QUEUES (FOR LATER)
//*************************************************