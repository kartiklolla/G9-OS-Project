// kernel/mutex.h
// Written by K-Mohan26
//
// xv6 already has locks called spinlocks in kernel/spinlock.c
// BUT the problem is those locks only work inside the kernel itself.
// Normal user programs cannot use them at all.
// Also those locks keep spinning in a loop burning CPU time.
// So we are making our own lock system that user programs can actually use.

#define MAX_MUTEXES 16   // we support up to 16 locks at a time

struct umutex {
  int valid;        // is this slot being used? 1=yes 0=no
  int locked;       // is this lock currently taken? 1=yes 0=no
  int owner_pid;    // which process is holding this lock right now
};

extern struct umutex mutextable[MAX_MUTEXES];
void mutexinit(void);
