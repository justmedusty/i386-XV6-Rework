//
// Created by dustyn on 6/5/24.
//

#ifndef I386_XV6_REWORK_SCHED_H
#define I386_XV6_REWORK_SCHED_H
#include "../defs/param.h"
#include "../data/queue.h"
//one for each possible cpu, only use one per CPU based off num_cpu result from mp.c
extern struct pqueue runqueue[NCPU];
extern struct pqueue sleepqueue;
extern struct pqueue readyqueue;

void sched(void);

void yield(void);

void preempt(void);

uint32 get_cpu_avg();

void update_cpu_avg(uint32 ticks);

void init_cpu_avg_counter();

void scheduler(void);


#endif //I386_XV6_REWORK_SCHED_H
