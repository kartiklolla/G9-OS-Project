#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define MUTEX_ID 0
#define SHM_KEY 88
#define WORKERS 4
#define ITERATIONS 2000

struct shared_counter {
  int value;
};

int
main(void)
{
  struct shared_counter *counter;
  int i;
  int pid;

  if(mutex_init(MUTEX_ID) < 0){
    fprintf(2, "mutexdemo: mutex_init failed\n");
    exit(1);
  }

  uint64 addr = shm_open(SHM_KEY, sizeof(struct shared_counter));
  if((long)addr < 0){
    fprintf(2, "mutexdemo: shm_open failed\n");
    exit(1);
  }

  counter = (struct shared_counter*)addr;
  counter->value = 0;

  for(i = 0; i < WORKERS; i++){
    pid = fork();
    if(pid < 0){
      fprintf(2, "mutexdemo: fork failed\n");
      shm_close(SHM_KEY);
      exit(1);
    }

    if(pid == 0){
      uint64 child_addr = shm_open(SHM_KEY, sizeof(struct shared_counter));
      if((long)child_addr < 0){
        fprintf(2, "mutexdemo: child shm_open failed\n");
        exit(1);
      }

      counter = (struct shared_counter*)child_addr;

      for(int j = 0; j < ITERATIONS; j++){
        if(mutex_lock(MUTEX_ID) < 0){
          fprintf(2, "mutexdemo: mutex_lock failed\n");
          exit(1);
        }
        counter->value++;
        if(mutex_unlock(MUTEX_ID) < 0){
          fprintf(2, "mutexdemo: mutex_unlock failed\n");
          exit(1);
        }
      }

      shm_close(SHM_KEY);
      exit(0);
    }
  }

  for(i = 0; i < WORKERS; i++)
    wait(0);

  printf("mutexdemo: final counter=%d expected=%d\n",
         counter->value,
         WORKERS * ITERATIONS);

  if(counter->value == WORKERS * ITERATIONS)
    printf("mutexdemo: PASSED\n");
  else
    printf("mutexdemo: FAILED\n");

  shm_close(SHM_KEY);
  exit(counter->value == WORKERS * ITERATIONS ? 0 : 1);
}
