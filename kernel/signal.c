#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "elf.h"

uint64 sys_sigalarm(void)
{
    struct proc *p = myproc();
    int interval;
    uint64 handler_addr;
    if(argint(0, &interval) < 0 || argaddr(1, &handler_addr) < 0)
        return -1;
    
    p->tick = 0;
    p->interval = interval;
    p->alarmhandle = (void (*)()) handler_addr;
    
    return 0;
}

uint64 sys_sigreturn(void)
{
    struct proc *p = myproc();

    memmove(p->tf, p->oldtf, sizeof(struct trapframe));
    kfree(p->oldtf);
    p->oldtf = 0;

    return 0;
}