#ifndef SHM_H
#define SHM_H

#define SHM_SLOTS   8   // max concurrent shared regions system-wide
#define SHM_MAXPGS  4   // max pages per region (16 KiB)

// One shared memory region.
struct shmregion {
  int    key;                    // user-chosen identifier; 0 = slot free
  int    npages;                 // number of physical pages allocated
  uint64 pages[SHM_MAXPGS];     // physical addresses of the pages
  int    refcount;               // number of processes currently attached
};

// Per-process attachment record (stored in struct proc).
// Tracks where each region is mapped in this process's address space.
#define PROC_SHM_SLOTS 8
struct shmattach {
  int    key;   // 0 = slot unused
  uint64 uva;   // user virtual address where mapped
  int    npages;
};

#endif
