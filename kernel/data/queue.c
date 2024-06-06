//
// Created by dustyn on 6/5/24.
//

#include "queue.h"
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

/*
 * We will create some reusable functions for dealing with proc queues and other types of queues in order to make it simpler to deal with many different
 * data structures, can reuse the wrapper functions defined here
 */

//*************************************************************
//PROC QUEUES
//*************************************************************
void initprocqueue(struct pqueue *procqueue) {
    initlock(&procqueue.qloc, "procqueue");
    procqueue.head = 0;
    procqueue.tail = 0;
}
int is_queue_empty(struct pqueue *procqueue) {
    int result = (procqueue.head == 0);
    return result;
}

int is_proc_alone_in_queue(struct proc *p,struct pqueue *procqueue){
    return (procqueue.head == p && procqueue.head->next == 0);
}

/*
 * This will traverse the queue , comparing priority, cpu usage against time quantum, and insert
 * the new process in an appropriate place in the queue. If there is nothing in the queue, it will be placed between head and tail.
 */
void insert_proc_into_queue(struct proc *new,struct pqueue *procqueue){
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
int is_proc_queued(struct proc *p,struct pqueue *procqueue) {

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
void remove_proc_from_queue(struct proc *old,struct pqueue *procqueue) {

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
void purge_queue(struct pqueue *procqueue) {

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
void shift_queue(struct pqueue *procqueue) {

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