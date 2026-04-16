#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define SHM_KEY 77

struct shared_state {
  int ready;
  int done;
  int counter;
  char message[64];
};

int
main(void)
{
  struct shared_state *sh;
  int pid;
  uint64 addr;

  addr = shm_open(SHM_KEY, sizeof(struct shared_state));
  if((long)addr < 0){
    fprintf(2, "shmdemo: shm_open failed\n");
    exit(1);
  }

  sh = (struct shared_state*)addr;
  sh->ready = 0;
  sh->done = 0;
  sh->counter = 10;
  strcpy(sh->message, "shared memory demo");

  pid = fork();
  if(pid < 0){
    fprintf(2, "shmdemo: fork failed\n");
    shm_close(SHM_KEY);
    exit(1);
  }

  if(pid == 0){
    uint64 child_addr = shm_open(SHM_KEY, sizeof(struct shared_state));
    if((long)child_addr < 0){
      fprintf(2, "shmdemo: child shm_open failed\n");
      exit(1);
    }

    sh = (struct shared_state*)child_addr;

    while(sh->ready == 0)
      ;

    sh->counter += 5;
    strcpy(sh->message, "child updated the shared counter");
    sh->done = 1;

    printf("child: counter=%d message=%s\n", sh->counter, sh->message);
    shm_close(SHM_KEY);
    exit(0);
  }

  sh->ready = 1;
  while(sh->done == 0)
    ;

  printf("parent: counter=%d message=%s\n", sh->counter, sh->message);
  wait(0);
  shm_close(SHM_KEY);

  exit(0);
}
