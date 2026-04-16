#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/procinfo.h"
#include "user/user.h"

static char *statenames[] = {
  "UNUSED",
  "USED",
  "SLEEPING",
  "RUNNABLE",
  "RUNNING",
  "ZOMBIE"
};

static char *
statename(int state)
{
  if(state < 0 || state > 5)
    return "?";
  return statenames[state];
}

int
main(void)
{
  struct procinfo info;
  int found;
  int pid;

  printf("PID PPID STATE    SZ     NAME\n");
  found = 0;

  for(pid = 1; pid < 128; pid++){
    if(getprocinfo(pid, &info) == 0){
      printf("%d %d %s %d %s\n",
             info.pid,
             info.ppid,
             statename(info.state),
             (int)info.sz,
             info.name);
      found = 1;
    }
  }

  if(!found)
    printf("ps: no live processes found\n");

  exit(0);
}
