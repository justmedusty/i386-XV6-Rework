#include "../../user/types.h"
#include "../arch/x86_32/x86.h"
#include "../defs/defs.h"
#include "../defs/date.h"
#include "../defs/param.h"
#include "../arch/x86_32/mem/memlayout.h"
#include "../arch/x86_32/mem/mmu.h"
#include "../lock/spinlock.h"
#include "../sched/proc.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}
int
sys_freemem(void){
    int pages = freemem();
    return pages;
}

int
sys_sig(void){
    int signal,pid;
    if((argint(0, &signal) < 0) || (argint(1, &pid) < 0)){
        return -1;
    }

    int result = sig(signal,pid);
    return result;
}

int
sys_sighandler(void) {
    void *sig_handler;
    if (argptr(1, (char **)&sig_handler, sizeof(void *)) < 0) {
        return -1; // Error: Failed to extract pointer argument
    }
    sighandler(*(void(**)(int))sig_handler);
    return 0;
}
int
sys_sigignore(void){
    int arg1, arg2;

    if(argint(0,&arg1) < 0){
        return -1;
    }
    if(argint(1,&arg2) < 0){
        return -1;
    }
    sigignore(arg1,arg2);
    return 0;

}

int
sys_sleep(void)
{
  int n;
  uint32 ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick trap have occurred
// since start.
int
sys_uptime(void)
{
  uint32 xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}
