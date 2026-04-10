#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "shm.h"
#include "proc.h"
#include "defs.h"

// System-wide table of shared memory regions.
static struct shmregion shm_table[SHM_SLOTS];
static struct spinlock  shm_lock;

void
shminit(void)
{
  initlock(&shm_lock, "shm");
  for(int i = 0; i < SHM_SLOTS; i++)
    shm_table[i].key = 0;
}

// shm_open(key, size):
//   Attach to (or create) the shared region identified by key.
//   size is rounded up to a whole number of pages (max SHM_MAXPGS pages).
//   Maps the region into the calling process's address space at p->sz.
//   Returns the user virtual address on success, -1 on error.
uint64
shm_open(int key, int size)
{
  struct proc *p = myproc();
  struct shmregion *slot = 0;
  int npages, i;
  uint64 uva;

  if(key <= 0 || size <= 0)
    return -1;

  // Round size up to whole pages; clamp to SHM_MAXPGS.
  npages = (size + PGSIZE - 1) / PGSIZE;
  if(npages > SHM_MAXPGS)
    npages = SHM_MAXPGS;

  acquire(&shm_lock);

  // Try to find an existing region with this key.
  for(i = 0; i < SHM_SLOTS; i++){
    if(shm_table[i].key == key){
      slot = &shm_table[i];
      break;
    }
  }

  // If not found, allocate a new slot.
  if(slot == 0){
    for(i = 0; i < SHM_SLOTS; i++){
      if(shm_table[i].key == 0){
        slot = &shm_table[i];
        slot->key      = key;
        slot->npages   = npages;
        slot->refcount = 0;
        // Allocate physical pages.
        for(int j = 0; j < npages; j++){
          void *pa = kalloc();
          if(pa == 0){
            // Roll back previously allocated pages.
            for(int k = 0; k < j; k++)
              kfree((void*)slot->pages[k]);
            slot->key = 0;
            release(&shm_lock);
            return -1;
          }
          memset(pa, 0, PGSIZE);
          slot->pages[j] = (uint64)pa;
        }
        break;
      }
    }
    if(slot == 0){
      release(&shm_lock);
      return -1;  // table full
    }
  }

  // Check this process isn't already attached to this key.
  for(i = 0; i < PROC_SHM_SLOTS; i++){
    if(p->shm[i].key == key){
      release(&shm_lock);
      return p->shm[i].uva;  // already mapped: return existing address
    }
  }

  // Find a free per-process slot.
  int pslot = -1;
  for(i = 0; i < PROC_SHM_SLOTS; i++){
    if(p->shm[i].key == 0){ pslot = i; break; }
  }
  if(pslot < 0){
    release(&shm_lock);
    return -1;  // process has too many shm attachments
  }

  // Map at p->sz (must be page-aligned already — guaranteed by allocproc/growproc).
  uva = PGROUNDUP(p->sz);
  uint64 map_size = (uint64)slot->npages * PGSIZE;

  // Ensure we don't collide with TRAPFRAME.
  if(uva + map_size > TRAPFRAME){
    release(&shm_lock);
    return -1;
  }

  for(i = 0; i < slot->npages; i++){
    if(mappages(p->pagetable, uva + (uint64)i*PGSIZE, PGSIZE,
                slot->pages[i], PTE_R | PTE_W | PTE_U) < 0){
      // Unmap pages mapped so far.
      uvmunmap(p->pagetable, uva, i, 0);
      release(&shm_lock);
      return -1;
    }
  }

  p->sz = uva + map_size;
  p->shm[pslot].key    = key;
  p->shm[pslot].uva    = uva;
  p->shm[pslot].npages = slot->npages;
  slot->refcount++;

  release(&shm_lock);
  return uva;
}

// shm_close(key):
//   Detach the calling process from the shared region with the given key.
//   If the last process detaches, the physical pages are freed.
//   Returns 0 on success, -1 if not attached.
int
shm_close(int key)
{
  struct proc *p = myproc();
  struct shmregion *slot = 0;
  int i;

  if(key <= 0)
    return -1;

  acquire(&shm_lock);

  // Find per-process attachment.
  int pslot = -1;
  for(i = 0; i < PROC_SHM_SLOTS; i++){
    if(p->shm[i].key == key){ pslot = i; break; }
  }
  if(pslot < 0){
    release(&shm_lock);
    return -1;  // not attached
  }

  // Find the global slot.
  for(i = 0; i < SHM_SLOTS; i++){
    if(shm_table[i].key == key){ slot = &shm_table[i]; break; }
  }
  if(slot == 0){
    release(&shm_lock);
    return -1;
  }

  // Unmap pages from this process (do_free=0: don't free physical memory).
  uvmunmap(p->pagetable, p->shm[pslot].uva, p->shm[pslot].npages, 0);

  // Clear per-process attachment.
  p->shm[pslot].key    = 0;
  p->shm[pslot].uva    = 0;
  p->shm[pslot].npages = 0;

  slot->refcount--;
  if(slot->refcount <= 0){
    // Last detach: free physical pages and clear the global slot.
    for(i = 0; i < slot->npages; i++)
      kfree((void*)slot->pages[i]);
    slot->key      = 0;
    slot->npages   = 0;
    slot->refcount = 0;
  }

  release(&shm_lock);
  return 0;
}
