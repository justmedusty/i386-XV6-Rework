//
// Created by dustyn on 6/5/24.
//

#ifndef I386_XV6_REWORK_SCHED_H
#define I386_XV6_REWORK_SCHED_H
#include "../defs/param.h"

struct pqueue {
    struct spinlock qloc;
    struct proc *head;
    struct proc *tail;
};

//one for each possible cpu, only use one per CPU based off num_cpu result from mp.c
extern struct pqueue procqueue[NCPU];

int is_proc_alone_in_queue(struct proc *p);

int is_queue_empty();

void insert_proc_into_queue(struct proc *new);

int is_proc_queued(struct proc *p);

void remove_proc_from_queue(struct proc *old);

void purge_queue();

void shift_queue();

void sched(void);

void yield(void);

void preempt(void);

uint get_cpu_avg();

void update_cpu_avg(uint ticks);

void init_cpu_avg_counter();

void scheduler(void);


#endif //I386_XV6_REWORK_SCHED_H
