#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

// waitstat: exercise the wait_stat() syscall.
//
// Forks a child that spins for a while (accumulates rtime),
// then the parent calls wait_stat() to collect timing info.

int
main(void)
{
  int wtime, rtime, status;
  int child;

  printf("test_wait_stat: start\n");

  child = fork();
  if(child == 0){
    // Child: burn some CPU ticks so rtime > 0.
    for(volatile long i = 0; i < 30000000L; i++)
      ;
    exit(42);
  }

  // Parent: wait for child and collect stats.
  wtime = 0; rtime = 0; status = 0;
  int pid = wait_stat(&wtime, &rtime, &status);

  if(pid < 0){
    printf("  wait_stat failed\n");
    exit(1);
  }

  printf("  child pid   : %d\n", pid);
  printf("  exit status : %d (expected 42)\n", status);
  printf("  rtime (ticks running)  : %d\n", rtime);
  printf("  wtime (ticks runnable) : %d\n", wtime);

  if(status != 42){
    printf("  FAILED: wrong exit status\n");
    exit(1);
  }
  if(rtime == 0){
    printf("  WARNING: rtime is 0 (child may have been too fast for tick resolution)\n");
  }

  printf("test_wait_stat: PASSED\n");
  exit(0);
}
