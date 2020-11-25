// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define NBUCKET 13

struct {
  struct spinlock lock[NBUCKET];
  struct buf buf[NBUF];
  struct buf buckets[NBUCKET];

  // Linked list of all buffers, through prev/next.
  // head.next is most recently used.
  // struct buf head;
} bcache;

int bhash(uint blockno)
{
  return blockno % NBUCKET;
}

void
binit(void)
{
  struct buf *b;

  for(int i=0; i<NBUCKET; i++){
    initlock(&bcache.lock[i], "bcache");
    b = &bcache.buckets[i];
    b->prev = b;
    b->next = b;
  }

  // Create linked list of buffers
  // bcache.head.prev = &bcache.head;
  // bcache.head.next = &bcache.head;
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = bcache.buckets[0].next;
    b->prev = &bcache.buckets[0];
    initsleeplock(&b->lock, "buffer");
    bcache.buckets[0].next->prev = b;
    bcache.buckets[0].next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
// static struct buf*
// bget(uint dev, uint blockno)
// {
//   struct buf *b;

//   acquire(&bcache.lock);

//   // Is the block already cached?
//   for(b = bcache.head.next; b != &bcache.head; b = b->next){
//     if(b->dev == dev && b->blockno == blockno){
//       b->refcnt++;
//       release(&bcache.lock);
//       acquiresleep(&b->lock);
//       return b;
//     }
//   }

//   // Not cached; recycle an unused buffer.
//   for(b = bcache.head.prev; b != &bcache.head; b = b->prev){
//     if(b->refcnt == 0) {
//       b->dev = dev;
//       b->blockno = blockno;
//       b->valid = 0;
//       b->refcnt = 1;
//       release(&bcache.lock);
//       acquiresleep(&b->lock);
//       return b;
//     }
//   }
//   panic("bget: no buffers");
// }

static struct buf* bget(uint dev, uint blockno)
{
  struct buf *b;
  uint hash = bhash(blockno);

  acquire(&bcache.lock[hash]);

  for(b = bcache.buckets[hash].next; b != &bcache.buckets[hash]; b = b->next) {
    if(b->dev == dev && b->blockno == blockno) {
      b->refcnt++;
      release(&bcache.lock[hash]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  release(&bcache.lock[hash]);

  uint next = bhash(hash+1);
  while(next != hash) {
    acquire(&bcache.lock[next]);

    for(b = bcache.buckets[next].prev; b != &bcache.buckets[next]; b = b->prev) {
      if(b->refcnt == 0) {
        b->dev = dev;
        b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;
        b->next->prev = b->prev;
        b->prev->next = b->next;
        release(&bcache.lock[next]);

        acquire(&bcache.lock[hash]);
        b->next = bcache.buckets[hash].next;
        b->prev = &bcache.buckets[hash];
        bcache.buckets[hash].next->prev = b;
        bcache.buckets[hash].next = b;
        release(&bcache.lock[hash]);
        acquiresleep(&b->lock);
        return b;
      }
    }

    release(&bcache.lock[next]);
    next = bhash(next+1);
  }

  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b->dev, b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b->dev, b, 1);
}

// Release a locked buffer.
// Move to the head of the MRU list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  int hash = bhash(b->blockno);

  acquire(&bcache.lock[hash]);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.buckets[hash].next;
    b->prev = &bcache.buckets[hash];
    bcache.buckets[hash].next->prev = b;
    bcache.buckets[hash].next = b;
  }
  release(&bcache.lock[hash]);
}

void
bpin(struct buf *b) {
  int hash = bhash(b->blockno);
  acquire(&bcache.lock[hash]);
  b->refcnt++;
  release(&bcache.lock[hash]);
}

void
bunpin(struct buf *b) {
  int hash = bhash(b->blockno);
  acquire(&bcache.lock[hash]);
  b->refcnt--;
  release(&bcache.lock[hash]);
}


