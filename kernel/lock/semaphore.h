//
// Created by dustyn on 6/11/24.
//

#ifndef I386_XV6_REWORK_SEMAPHORE_H
#define I386_XV6_REWORK_SEMAPHORE_H
#define EINVL 1
#define ECANTINSERT 2
#define MAX_HOLDERS 10 //we'll set an arbitrary max holders
#define MAX_SEM_VAL 16
struct semaphore{
    int sem_value;
    int sem_waiting; // number waiting on this semaphore, this can be a queue of either exclusive or non-exclusive waiters but now this is fine
    int holding;
    int holder_pids[MAX_HOLDERS]; // this will be a way to verify that the process calling sem_inc actually has it and we can see who is holding it
    struct spinlock *lk;
};
void sem_inc(struct semaphore *sem);
int init_sem(struct semaphore *sem, int sem_value);
int sem_dec(struct semaphore *sem);
#endif //I386_XV6_REWORK_SEMAPHORE_H
