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

void sched(void);

void yield(void);

void preempt(void);

uint get_cpu_avg();

void update_cpu_avg(uint ticks);

void init_cpu_avg_counter();

void scheduler(void);


#endif //I386_XV6_REWORK_SCHED_H
