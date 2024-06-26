//
// Created by dustyn on 6/5/24.
//

#ifndef I386_XV6_REWORK_QUEUE_H
#define I386_XV6_REWORK_QUEUE_H
struct pqueue {
    struct spinlock qloc;
    struct proc *head;
    struct proc *tail;
    int len;
};
//proc queues
void initprocqueue(struct pqueue *procqueue);
int is_queue_empty(struct pqueue *procqueue);
int is_proc_alone_in_queue(struct proc *p,struct pqueue *procqueue);
void insert_proc_into_queue(struct proc *new,struct pqueue *procqueue);
int is_proc_queued(struct proc *p,struct pqueue *procqueue);
void remove_proc_from_queue(struct proc *old,struct pqueue *procqueue);
int claim_proc(struct proc *p,int cpu);
int unclaim_proc(struct proc *p);
void purge_queue(struct pqueue *procqueue);
void shift_queue(struct pqueue *procqueue);
unsigned char queues_need_balance();
void do_balance(unsigned char rq_mask);
#endif //I386_XV6_REWORK_QUEUE_H
