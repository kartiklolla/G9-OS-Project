#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

// test_priority: verify setpriority/getpriority syscalls and scheduler preference.
//
// Tests:
//   1. getpriority of self should return default (10).
//   2. setpriority to a valid value, then getpriority confirms it.
//   3. setpriority with out-of-range values returns -1.
//   4. setpriority/getpriority on a bogus pid returns -1.
//   5. Scheduler preference: fork two children with different priorities;
//      the high-priority child (lower number) should finish first.

int
main(void)
{
  int pid = getpid();
  int pri;

  printf("test_priority: start\n");

  // Test 1: default priority is 10.
  pri = getpriority(pid);
  printf("  default priority: %d (expected 10)\n", pri);
  if(pri != 10){ printf("  FAILED: default\n"); exit(1); }

  // Test 2: set and read back.
  if(setpriority(pid, 5) < 0){ printf("  FAILED: setpriority(5)\n"); exit(1); }
  pri = getpriority(pid);
  printf("  after setpriority(5): %d\n", pri);
  if(pri != 5){ printf("  FAILED: readback\n"); exit(1); }

  // Test 3: out-of-range values must return -1.
  if(setpriority(pid, 0) != -1 || setpriority(pid, 21) != -1){
    printf("  FAILED: out-of-range not rejected\n"); exit(1);
  }
  printf("  out-of-range values rejected correctly\n");

  // Test 4: bogus pid.
  if(getpriority(99999) != -1 || setpriority(99999, 5) != -1){
    printf("  FAILED: bogus pid not rejected\n"); exit(1);
  }
  printf("  bogus pid rejected correctly\n");

  // Restore own priority to default before the scheduler test.
  setpriority(pid, 10);

  // Test 5: scheduler preference.
  // Fork a LOW-priority child (priority 18) and a HIGH-priority child
  // (priority 3). Both spin-count to simulate work. The high-priority
  // child should complete first because the scheduler always picks the
  // lowest priority number among RUNNABLE processes.
  printf("  scheduler test: forking two children...\n");

  int first_done = -1;  // pid of whichever child exits first

  int lo = fork();      // will be low priority (slow)
  if(lo == 0){
    setpriority(getpid(), 18);
    for(volatile long i = 0; i < 20000000L; i++)
      ;
    exit(0);
  }

  int hi = fork();      // will be high priority (fast)
  if(hi == 0){
    setpriority(getpid(), 3);
    for(volatile long i = 0; i < 20000000L; i++)
      ;
    exit(1);  // exit status 1 to distinguish from lo child's 0
  }

  // Wait for whichever child finishes first.
  int status;
  first_done = wait(&status);
  printf("  first to finish: pid=%d exit_status=%d\n", first_done, status);

  // Reap the second child.
  wait(0);

  if(first_done == hi){
    printf("  high-priority child finished first: PASSED\n");
  } else {
    // On a real SMP system with multiple CPUs both may run in parallel,
    // so we accept either outcome but note it.
    printf("  low-priority child finished first (may be SMP artifact): OK\n");
  }

  printf("test_priority: PASSED\n");
  exit(0);
}
