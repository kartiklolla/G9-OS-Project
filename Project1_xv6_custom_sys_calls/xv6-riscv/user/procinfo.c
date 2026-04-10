#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/procinfo.h"
#include "user/user.h"

static const char *statenames[] = {
  "UNUSED", "USED", "SLEEPING", "RUNNABLE", "RUNNING", "ZOMBIE"
};

int
main(void)
{
  struct procinfo info;
  int pid = getpid();

  printf("test_getprocinfo: start\n");

  // Query our own process.
  if(getprocinfo(pid, &info) < 0){
    printf("  getprocinfo(%d) failed\n", pid);
    exit(1);
  }

  const char *state = (info.state >= 0 && info.state <= 5)
                      ? statenames[info.state] : "?";
  printf("  self  -> pid=%d ppid=%d state=%s sz=%d name=%s\n",
         info.pid, info.ppid, state, (int)info.sz, info.name);

  if(info.pid != pid){
    printf("  FAILED: returned pid %d, expected %d\n", info.pid, pid);
    exit(1);
  }

  // Query a nonexistent pid — should return -1.
  if(getprocinfo(99999, &info) == 0){
    printf("  FAILED: expected -1 for bogus pid\n");
    exit(1);
  }
  printf("  bogus pid returned -1 as expected\n");

  // Fork a child, query it from parent.
  int child = fork();
  if(child == 0){
    // Child: spin briefly so parent can query it.
    for(volatile int i = 0; i < 100000; i++)
      ;
    exit(0);
  }

  if(getprocinfo(child, &info) == 0){
    const char *cs = (info.state >= 0 && info.state <= 5)
                     ? statenames[info.state] : "?";
    printf("  child -> pid=%d ppid=%d state=%s name=%s\n",
           info.pid, info.ppid, cs, info.name);
  }

  wait(0);
  printf("test_getprocinfo: PASSED\n");
  exit(0);
}
