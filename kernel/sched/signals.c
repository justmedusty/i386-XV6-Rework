//
// Created by dustyn on 6/9/24.
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


int signals_pending(struct proc *p) {
    return (p->p_sig != 0);
}

void handle_signals(struct proc *p) {
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
