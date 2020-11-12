// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
  uint *ref_count;
} kmem;

uint kref(void *pa)
{
  uint index = (((uint64*)pa - (uint64*)end) >> 12);
  acquire(&kmem.lock);
  kmem.ref_count[index]++;
  release(&kmem.lock);
  return kmem.ref_count[index];
}

uint kderef(void *pa)
{
  uint index = (((uint64*)pa - (uint64*)end) >> 12);
  acquire(&kmem.lock);
  kmem.ref_count[index]--;
  release(&kmem.lock);
  return kmem.ref_count[index];
}

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  // freerange(end, (void*)PHYSTOP);
  kmem.ref_count = (uint *) end;
  uint64 rc_offset = (((((PHYSTOP - (uint64)end) >> 12) + 1) * sizeof(uint) >> 12) + 1) << 12;
  memset(end, 0, rc_offset);
  freerange(end + rc_offset, (void *) PHYSTOP);
  // printf("kinit\n");
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  struct run *r;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE){
    // kfree(p);
    r = (struct run*)p;
    memset(p, 1, PGSIZE);
    acquire(&kmem.lock);
    r->next = kmem.freelist;
    kmem.freelist = r;
    release(&kmem.lock);
  }
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  // printf("kfree\n");
  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  // memset(pa, 1, PGSIZE);

  r = (struct run*)pa;


  uint ref = kderef(pa);
  if(ref == 0){
    memset(pa, 1, PGSIZE);
    acquire(&kmem.lock);
    r->next = kmem.freelist;
    kmem.freelist = r;
    release(&kmem.lock);
  }

  // acquire(&kmem.lock);
  // r->next = kmem.freelist;
  // kmem.freelist = r;
  // release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;
  
  // printf("kalloc\n");
  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r){
    memset((char*)r, 5, PGSIZE); // fill with junk
    uint64 index = (((uint64 *)r - (uint64 *)end) >> 12);
    acquire(&kmem.lock);
    kmem.ref_count[index] = 1;
    release(&kmem.lock);
  }

  // printf("kalloc already\n");
    // memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
