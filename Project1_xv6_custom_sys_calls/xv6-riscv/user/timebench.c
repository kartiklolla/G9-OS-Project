#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

static void
usage(void)
{
  fprintf(2, "usage: timebench command [args...]\n");
  exit(1);
}

int
main(int argc, char *argv[])
{
  int pid;
  int wtime;
  int rtime;
  int status;

  if(argc < 2)
    usage();

  pid = fork();
  if(pid < 0){
    fprintf(2, "timebench: fork failed\n");
    exit(1);
  }

  if(pid == 0){
    exec(argv[1], &argv[1]);
    fprintf(2, "timebench: exec %s failed\n", argv[1]);
    exit(1);
  }

  wtime = 0;
  rtime = 0;
  status = 0;

  pid = wait_stat(&wtime, &rtime, &status);
  if(pid < 0){
    fprintf(2, "timebench: wait_stat failed\n");
    exit(1);
  }

  printf("timebench: command=%s pid=%d status=%d\n", argv[1], pid, status);
  printf("  runnable ticks : %d\n", wtime);
  printf("  running ticks   : %d\n", rtime);
  printf("  total ticks     : %d\n", wtime + rtime);

  exit(0);
}
