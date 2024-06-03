#include "../defs/types.h"
#include "../defs/defs.h"
#include "../defs/param.h"
#include "../arch/x86_32/mem/memlayout.h"
#include "../arch/x86_32/mem/mmu.h"
#include "../sched/proc.h"
#include "../arch/x86_32/x86.h"
#include "../arch/x86_32/traps.h"
#include "../lock/spinlock.h"
#include "../sched/signal.h"
// Interrupt descriptor table (shared by all CPUs).
struct gatedesc idt[256];
extern uint vectors[];  // in vectors.S: array of 256 entry pointers
struct spinlock tickslock;
uint ticks;

void
tvinit(void) {
    int i;

    for (i = 0; i < 256; i++) SETGATE(idt[i], 0, SEG_KCODE << 3, vectors[i], 0);
    SETGATE(idt[T_SYSCALL], 1, SEG_KCODE << 3, vectors[T_SYSCALL], DPL_USER);

    initlock(&tickslock, "time");
}

void
idtinit(void) {
    lidt(idt, sizeof(idt));
}

//PAGEBREAK: 41
void
trap(struct trapframe *tf) {
    if (tf->trapno == T_SYSCALL) {
        if (myproc()->killed)
            exit();
        myproc()->tf = tf;
        syscall();
        if (myproc()->killed)
            exit();
        return;
    }

    switch (tf->trapno) {
        case T_PGFLT:
            panic("PAGE FAULT");
            uint addr = rcr2();
            if(myproc() && addr < myproc()->sz && addr >= myproc()->stack_base - MAXSTACKSIZE){
                // Check if the faulting address is within the stack growth range
                uint newstacksize = PGROUNDUP(addr);
                if(allocuvm(myproc()->pgdir, myproc()->stack_base - PGSIZE, newstacksize,0) == 0) {
                    cprintf("allocuvm failed for stack growth\n");
                    myproc()->p_sig |= SIGSEG;
                } else {
                    myproc()->stack_base = newstacksize; // Update the stack base
                }
                return;
            }

        case T_IRQ0 + IRQ_TIMER:
            if (cpuid() == 0) {
                acquire(&tickslock);
                ticks++;
                wakeup(&ticks);
                release(&tickslock);
            }
            lapiceoi();
            break;
            //We will swap this out with a real handler once we implement our new drivers
        case T_IRQ0 + IRQ_IDE:
            ideintr();
            lapiceoi();
            break;
        case T_IRQ0 + IRQ_IDE2:
            ideintr2();
            lapiceoi();
            break;
        case T_IRQ0 + IRQ_KBD:
            kbdintr();
            lapiceoi();
            break;
        case T_IRQ0 + IRQ_COM1:
            uartintr();
            lapiceoi();
            break;
        case T_IRQ0 + 7:
        case T_IRQ0 + IRQ_SPURIOUS:
            cprintf("cpu%d: spurious interrupt at %x:%x\n",
                    cpuid(), tf->cs, tf->eip);
            lapiceoi();
            break;

            //PAGEBREAK: 13
        default:
            if (myproc() == 0 || (tf->cs & 3) == 0) {
                // In kernel, it must be our mistake.
                cprintf("unexpected trap %d from cpu %d eip %x (cr2=0x%x)\n",
                        tf->trapno, cpuid(), tf->eip, rcr2());
                panic("trap");
            }
            // In user space, assume process misbehaved.
            cprintf("pid %d %s: trap %d err %d on cpu %d "
                    "eip 0x%x addr 0x%x--kill proc\n",
                    myproc()->pid, myproc()->name, tf->trapno,
                    tf->err, cpuid(), tf->eip, rcr2());
            myproc()->killed = 1;
    }

// Force process exit if it has been killed and is in user space.
// (If it is still executing in the kernel, let it keep running
// until it gets to the regular system call return.)
    if (myproc() && myproc()->killed && (tf->cs & 3) == DPL_USER)
        exit();

    // Force process to give up CPU on exceeding time quantum.
    // If trap were on while locks held, would need to check nlock.


    //Increment the cpu usage counter for the process each clock interrupt and check against its time quantum , preempt it if it exceeds it's given time quantum

    if (myproc() && myproc()->state == RUNNING && tf->trapno == T_IRQ0 + IRQ_TIMER) {
        myproc()->p_cpu_usage++;
    }

    if (myproc() && myproc()->state == RUNNING && myproc()->p_cpu_usage > myproc()->p_time_quantum) {
        /*
         * We will ensure the process that is exceeding its time quantum is not preempted if no other process is queued
         */
        if (!is_proc_alone_in_queue(myproc())) {
            preempt();
        }

    }

// Check if the process has been killed since we yielded
    if (myproc() && myproc()->killed && (tf->cs & 3) == DPL_USER)
        exit();

}