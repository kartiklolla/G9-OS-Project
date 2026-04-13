#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define MUTEX_ID 0

int
main(void)
{
  printf("=== mutex test ===\n");

  if(mutex_init(MUTEX_ID) < 0){
    printf("FAIL: mutex_init\n");
    exit(1);
  }
  printf("mutex_init(%d): OK\n", MUTEX_ID);

  if(mutex_lock(MUTEX_ID) < 0){
    printf("FAIL: mutex_lock\n");
    exit(1);
  }
  printf("mutex_lock(%d): OK (acquired)\n", MUTEX_ID);

  if(mutex_unlock(MUTEX_ID) < 0){
    printf("FAIL: mutex_unlock\n");
    exit(1);
  }
  printf("mutex_unlock(%d): OK (released)\n", MUTEX_ID);

  mutex_init(MUTEX_ID);
  mutex_lock(MUTEX_ID);
  printf("Parent (pid=%d) holds mutex.\n", getpid());

  int pid = fork();
  if(pid == 0){
    printf("Child (pid=%d) waiting for mutex...\n", getpid());
    mutex_lock(MUTEX_ID);
    printf("Child acquired mutex!\n");
    mutex_unlock(MUTEX_ID);
    printf("Child released mutex.\n");
    exit(0);
  }

  for(volatile int i = 0; i < 10000000; i++);
  printf("Parent releasing mutex.\n");
  mutex_unlock(MUTEX_ID);
  wait(0);
  printf("=== mutex test PASSED ===\n");
  exit(0);
}
