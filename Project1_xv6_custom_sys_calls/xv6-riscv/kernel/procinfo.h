#ifndef PROCINFO_H
#define PROCINFO_H

// Filled by getprocinfo(). State values match enum procstate in proc.h:
//   0=UNUSED 1=USED 2=SLEEPING 3=RUNNABLE 4=RUNNING 5=ZOMBIE
struct procinfo {
  int    pid;
  int    ppid;
  int    state;
  uint64 sz;
  char   name[16];
};

#endif
