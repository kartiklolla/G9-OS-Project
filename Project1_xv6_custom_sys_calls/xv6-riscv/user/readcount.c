#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

// test_getreadcount: verify that getreadcount() increments on each read().
//
// Strategy:
//   1. Record the current count (baseline).
//   2. Open a file and read from it N times.
//   3. Confirm the count rose by at least N.
//   We read from fd 0 (stdin) instead of a file so we don't need the
//   filesystem — but we pipe a known byte through a pipe so the read
//   actually succeeds.

int
main(void)
{
  int before, after;
  int fds[2];
  char buf[1];

  printf("test_getreadcount: start\n");

  before = getreadcount();
  printf("  read count before: %d\n", before);

  // Create a pipe and write 3 bytes so we can read them back.
  if(pipe(fds) < 0){
    printf("pipe failed\n");
    exit(1);
  }

  write(fds[1], "abc", 3);
  close(fds[1]);

  // Perform 3 reads.
  read(fds[0], buf, 1);
  read(fds[0], buf, 1);
  read(fds[0], buf, 1);
  close(fds[0]);

  after = getreadcount();
  printf("  read count after:  %d\n", after);
  printf("  delta: %d (expected >= 3)\n", after - before);

  if(after - before >= 3){
    printf("test_getreadcount: PASSED\n");
  } else {
    printf("test_getreadcount: FAILED\n");
    exit(1);
  }

  exit(0);
}
