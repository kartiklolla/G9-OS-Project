#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

static void
burn(long iters)
{
  for(volatile long i = 0; i < iters; i++)
    ;
}

int
main(void)
{
  int low;
  int high;
  int first;

  printf("scheddemo: run with CPUS=1 for the clearest result\n");

  low = fork();
  if(low < 0){
    fprintf(2, "scheddemo: fork failed\n");
    exit(1);
  }
  if(low == 0){
    setpriority(getpid(), 18);
    printf("low-priority child pid=%d priority=%d starting\n",
           getpid(), getpriority(getpid()));
    burn(20000000L);
    printf("low-priority child done\n");
    exit(0);
  }

  if(setpriority(low, 18) < 0){
    fprintf(2, "scheddemo: setpriority(low) failed\n");
    exit(1);
  }

  high = fork();
  if(high < 0){
    fprintf(2, "scheddemo: fork failed\n");
    exit(1);
  }
  if(high == 0){
    setpriority(getpid(), 3);
    printf("high-priority child pid=%d priority=%d starting\n",
           getpid(), getpriority(getpid()));
    burn(20000000L);
    printf("high-priority child done\n");
    exit(0);
  }

  if(setpriority(high, 3) < 0){
    fprintf(2, "scheddemo: setpriority(high) failed\n");
    exit(1);
  }

  first = wait(0);
  printf("scheddemo: first child to finish pid=%d\n", first);
  wait(0);
  printf("scheddemo: done\n");

  exit(0);
}
