#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#include "kernel/syscall.h"
#include "kernel/memlayout.h"
#include "kernel/riscv.h"

void
main(int args, char **argv)
{
  enum { BIG=100*1024*1024 };
  char *c, *oldbrk, *a, *lastaddr, *p;
  uint64 amt;

  oldbrk = sbrk(0);

  // can one grow address space to something big?
  a = sbrk(0);
  amt = BIG - (uint64)a;
  p = sbrk(amt);
  if (p != a) {
    printf("sbrk test failed to grow big address space; enough phys mem?\n");
    exit(1);
  }
  lastaddr = (char*) (BIG-1);
  *lastaddr = 99;

  // can one de-allocate?
  a = sbrk(0);
  c = sbrk(-PGSIZE);
  printf("a %x, c %x, ? %x\n", a, c, (char*)0xffffffffffffffffL);
  if(c == (char*)0xffffffffffffffffL){
    printf("a %x, c %x, ? %x\n", a, c, (char*)0xffffffffffffffffL);
    printf("sbrk could not deallocate\n");
    // exit(1);
  }
  c = sbrk(0);
  if(c != a - PGSIZE){
    printf("sbrk deallocation produced wrong address, a %x c %x a-PGSIZE %x\n", a, c);
    exit(1);
  }

  // can one re-allocate that page?
  a = sbrk(0);
  c = sbrk(PGSIZE);
  if(c != a || sbrk(0) != a + PGSIZE){
    printf("sbrk re-allocation failed, a %x c %x\n", a, c);
    exit(1);
  }
  if(*lastaddr == 99){
    // should be zero
    printf("sbrk de-allocation didn't really deallocate\n");
    exit(1);
  }

  a = sbrk(0);
  c = sbrk(-(sbrk(0) - oldbrk));
  if(c != a){
    printf("sbrk downsize failed, a %x c %x\n", a, c);
    exit(1);
  }
}