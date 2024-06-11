//
// Created by dustyn on 6/11/24.
//
#include "../../user/types.h"
#include "../defs/param.h"
#include "../arch/x86_32/mem/mmu.h"
#include "../defs/defs.h"
#include "../arch/x86_32/x86.h"
#include "spinlock.h"
#include "../sched/proc.h"
#include "semaphore.h"
static int remove_pid(int pid, struct semaphore *sem);
//increment semaphore value
void sem_inc(struct semaphore *sem) {
    struct proc *this = myproc();
    sem->sem_value++;
    sem->holding--;
    remove_pid(this->pid);
    if (sem->sem_waiting > 0) {
        wakeup(sem);
    }

}
//find a pid in the sem holders array to verify that they are actually there
static int remove_pid(int pid, struct semaphore *sem) {
    for (int i = 0; i < MAX_HOLDERS; i++) {
        if (sem->holder_pids[i] == pid) {
            sem->holder_pids[i] = 0;
            return 1;
        }
    }
    return 0;
}
//insert new pid into the holders array
static int insert_pid(int pid, struct semaphore *sem) {
    for (int i = 0; i < MAX_HOLDERS; i++) {
        if (sem->holder_pids[i] == 0) {
            sem->holder_pids[i] = pid;
            return 1;
        }
    }
    return 0;
}
//init sem with an initial value passed in , must be at least one and no greater than MAX_SEM_VAL
int init_sem(struct semaphore *sem, int sem_value) {
    if (sem_value <= 0 || sem_value > MAX_SEM_VAL) {
        return -EINVL;
    }
    sem->sem_waiting = 0;
    sem->holding = 0;
    sem->sem_value = sem_value;
    for (int i = 0; i < MAX_HOLDERS; i++) {
        sem->holder_pids[i] = 0;
    }
    return 1;
}
//decrement the semaphore, sleep if value is 0, otherwise update sem data strcuture accordingly
int sem_dec(struct semaphore *sem){
    struct proc *this_p = myproc();
    acquire(sem->lk);
    if(sem->sem_value == 0){
        sleep(sem,sem->lk);
    }
    if(!insert_pid(this_p->pid,sem)){
        return -ECANTINSERT;
    }
    sem->sem_value--;
    sem->holding++;
    release(sem->lk);
    return 0;
}