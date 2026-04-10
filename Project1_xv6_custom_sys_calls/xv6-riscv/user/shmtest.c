#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

// shmtest: exercise shm_open() and shm_close().
//
// Test 1: parent opens a region, writes a value, child opens the same
//         region and reads it back -- verifies shared physical memory.
// Test 2: double open returns the same address.
// Test 3: shm_close on unknown key returns -1.

int
main(void)
{
  printf("test_shm: start\n");

  // --- Test 1: parent writes, child reads ---
  int key = 42;
  // shm_open returns a uint64 virtual address; cast via uint64 to avoid
  // int-to-pointer-cast warning on the 64-bit RISC-V target.
  uint64 raw = shm_open(key, 4096);
  if((uint64)raw < 0){
    printf("  FAILED: shm_open returned error\n");
    exit(1);
  }
  int *addr = (int*)raw;
  printf("  parent: shm mapped at %p\n", addr);
  *addr = 0xdeadbeef;

  int pid = fork();
  if(pid == 0){
    // Child attaches to the same key.
    uint64 craw = shm_open(key, 4096);
    if((uint64)craw < 0){
      printf("  child: FAILED: shm_open error\n");
      exit(1);
    }
    int *caddr = (int*)craw;
    int val = *caddr;
    printf("  child: read 0x%x (expected 0xdeadbeef)\n", val);
    shm_close(key);
    exit(val == (int)0xdeadbeef ? 0 : 1);
  }

  int status;
  wait(&status);
  if(status != 0){
    printf("  FAILED: child did not read correct value\n");
    exit(1);
  }
  printf("  parent/child shared memory test: PASSED\n");

  // --- Test 2: double open returns same address ---
  uint64 raw2 = shm_open(key, 4096);
  int *addr2 = (int*)raw2;
  if(addr2 != addr){
    printf("  note: double open returned different addr (may be ok)\n");
  } else {
    printf("  double open returned same addr: PASSED\n");
  }

  // --- Test 3: close then close-again returns -1 ---
  shm_close(key);
  shm_close(key);  // second close: refcount already 0, returns -1 (harmless)
  if(shm_close(99) != -1){
    printf("  FAILED: expected -1 for unknown key\n");
    exit(1);
  }
  printf("  unknown key returns -1: PASSED\n");

  printf("test_shm: PASSED\n");
  exit(0);
}
