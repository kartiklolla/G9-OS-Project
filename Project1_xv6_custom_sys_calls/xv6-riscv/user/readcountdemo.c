#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

static void
usage(void)
{
  fprintf(2, "usage: readcountdemo [file]\n");
  exit(1);
}

int
main(int argc, char *argv[])
{
  int fd;
  int before;
  int after;
  int n;
  int bytes;
  int reads;
  char buf[128];

  if(argc > 2)
    usage();

  fd = 0;
  if(argc == 2){
    fd = open(argv[1], O_RDONLY);
    if(fd < 0){
      fprintf(2, "readcountdemo: cannot open %s\n", argv[1]);
      exit(1);
    }
  }

  before = getreadcount();
  bytes = 0;
  reads = 0;

  while((n = read(fd, buf, sizeof(buf))) > 0){
    bytes += n;
    reads++;
  }

  if(n < 0){
    fprintf(2, "readcountdemo: read failed\n");
    if(fd > 0)
      close(fd);
    exit(1);
  }

  if(fd > 0)
    close(fd);

  after = getreadcount();

  printf("readcountdemo: source=%s\n", argc == 2 ? argv[1] : "stdin");
  printf("  bytes read        : %d\n", bytes);
  printf("  read() calls used  : %d\n", reads);
  printf("  syscall counter    : %d -> %d (delta %d)\n", before, after, after - before);

  exit(0);
}
