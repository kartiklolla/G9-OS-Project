#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "vm.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  kexit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return kfork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return kwait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int t;
  int n;

  argint(0, &n);
  argint(1, &t);
  addr = myproc()->sz;

  if(t == SBRK_EAGER || n < 0) {
    if(growproc(n) < 0) {
      return -1;
    }
  } else {
    // Lazily allocate memory for this process: increase its memory
    // size but don't allocate memory. If the processes uses the
    // memory, vmfault() will allocate it.
    if(addr + n < addr)
      return -1;
    if(addr + n > TRAPFRAME)
      return -1;
    myproc()->sz += n;
  }
  return addr;
}

uint64
sys_pause(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  if(n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kkill(pid);
}

uint64
sys_setpriority(void)
{
  int pid, priority;
  argint(0, &pid);
  argint(1, &priority);
  return setpriority(pid, priority);
}

uint64
sys_getpriority(void)
{
  int pid;
  argint(0, &pid);
  return getpriority(pid);
}

uint64
sys_shm_open(void)
{
  int key, size;
  argint(0, &key);
  argint(1, &size);
  return shm_open(key, size);
}

uint64
sys_shm_close(void)
{
  int key;
  argint(0, &key);
  return shm_close(key);
}

uint64
sys_wait_stat(void)
{
  uint64 wtime, rtime, status;
  argaddr(0, &wtime);
  argaddr(1, &rtime);
  argaddr(2, &status);
  return kwait_stat(wtime, rtime, status);
}

uint64
sys_getprocinfo(void)
{
  int pid;
  uint64 addr;
  argint(0, &pid);
  argaddr(1, &addr);
  return getprocinfo(pid, addr);
}

uint64
sys_getreadcount(void)
{
  return getreadcount();
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

// =====================================================
// MUTEX LOCK SYSCALLS
// Written by K-Mohan26
// =====================================================
// Before writing this we looked at kernel/spinlock.c
// The existing lock there works like this:
//   while(locked != 0)
//      ;   <-- just sits here doing nothing, wasting CPU
// And it cant be called from user programs at all.
// Our version calls yield() instead of wasting CPU
// and user programs can call it using a simple ID number.
// =====================================================

#include "mutex.h"

// this is the actual table that stores all our mutexes in kernel memory
struct umutex mutextable[MAX_MUTEXES];

// mutexinit - runs once when xv6 boots up
// just sets everything to zero so the table starts clean
void
mutexinit(void)
{
  for(int i = 0; i < MAX_MUTEXES; i++){
    mutextable[i].valid     = 0;
    mutextable[i].locked    = 0;
    mutextable[i].owner_pid = 0;
  }
}

// sys_mutex_init - user calls mutex_init() to get a new lock
// we scan the table for a free slot and return its index
// that index is the "ID" the user will use from now on
uint64
sys_mutex_init(void)
{
  for(int i = 0; i < MAX_MUTEXES; i++){
    if(mutextable[i].valid == 0){
      mutextable[i].valid     = 1;
      mutextable[i].locked    = 0;
      mutextable[i].owner_pid = 0;
      return i;    // give the user this slot number
    }
  }
  return -1;   // all 16 slots are taken
}

// sys_mutex_lock - user calls mutex_lock(id) to take the lock
// if someone else already has it we wait by calling yield()
// yield() tells the CPU "go run something else for now"
// this is better than the existing spinlock which just burns CPU
uint64
sys_mutex_lock(void)
{
  int id;
  argint(0, &id);   // read the id argument the user passed in

  // basic checks - make sure the id makes sense
  if(id < 0 || id >= MAX_MUTEXES) return -1;
  if(mutextable[id].valid == 0)   return -1;

  struct umutex *m = &mutextable[id];

  // keep trying until we get the lock
  // __sync_lock_test_and_set sets locked=1 and returns old value
  // if old value was already 1 someone else had it so we wait
  while(__sync_lock_test_and_set(&m->locked, 1) != 0){
    yield();   // give up CPU while waiting, check again later
  }

  // we got the lock - remember who owns it
  m->owner_pid = myproc()->pid;
  return 0;
}

// sys_mutex_unlock - user calls mutex_unlock(id) to release the lock
// we check that only the process that locked it can unlock it
// then we clear the lock so someone else can take it
uint64
sys_mutex_unlock(void)
{
  int id;
  argint(0, &id);   // read the id argument

  if(id < 0 || id >= MAX_MUTEXES) return -1;
  if(mutextable[id].valid == 0)   return -1;

  // safety - only the owner can unlock
  if(mutextable[id].owner_pid != myproc()->pid) return -1;

  mutextable[id].owner_pid = 0;
  __sync_lock_release(&mutextable[id].locked);  // atomically clear the lock
  return 0;
}
