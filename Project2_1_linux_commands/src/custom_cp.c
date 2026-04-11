#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>
#include <limits.h>

#include "common.h"

#define BUF_SIZE 4096

void copy_file(const char *src, const char *dest, int interactive,
               int verbose) {
  struct stat st_src, st_dest;

  if (stat(src, &st_src) != 0) {
    warn("custom_cp", "Source stat failed");
    return;
  }

  char dest_path[PATH_MAX];
  strncpy(dest_path, dest, PATH_MAX - 1);
  dest_path[PATH_MAX - 1] = '\0';

  if (stat(dest, &st_dest) == 0) {
    if (S_ISDIR(st_dest.st_mode)) {
      char *src_copy = strdup(src);
      char *base = basename(src_copy);
      snprintf(dest_path, PATH_MAX, "%s/%s", dest, base);
      free(src_copy);

      // Re-stat the new destination path
      if (stat(dest_path, &st_dest) == 0) {
        if (st_src.st_dev == st_dest.st_dev && st_src.st_ino == st_dest.st_ino) {
          fprintf(stderr, "custom_cp: '%s' and '%s' are the same file\n", src, dest_path);
          return;
        }
      }
    } else {
      if (st_src.st_dev == st_dest.st_dev && st_src.st_ino == st_dest.st_ino) {
        fprintf(stderr, "custom_cp: '%s' and '%s' are the same file\n", src, dest_path);
        return;
      }
    }
  }

  //-i flag checks if destination exists or not
  if (interactive && access(dest_path, F_OK) == 0) {
    printf("custom_cp: Overwrite '%s'? (y/n): ", dest_path);
    char choice = getchar();
    if (choice != 'y' && choice != 'Y') {
      return;
    }
  }

  int srcfd = open(src, O_RDONLY);
  if (srcfd < 0) {
    warn("custom_cp", "Source open failed");
    return;
  }

  int destfd = open(dest_path, O_WRONLY | O_CREAT | O_TRUNC, st_src.st_mode);
  if (destfd < 0) {
    warn("custom_cp", "Destination open failed");
    close(srcfd);
    return;
  }

  char buf[BUF_SIZE];
  ssize_t bytes_read;

  while ((bytes_read = read(srcfd, buf, BUF_SIZE)) > 0) {
    ssize_t bytes_written = 0;
    while (bytes_written < bytes_read) {
      ssize_t w = write(destfd, buf + bytes_written, bytes_read - bytes_written);
      if (w < 0) {
        warn("custom_cp", "Write error");
        close(srcfd);
        close(destfd);
        return;
      }
      bytes_written += w;
    }
  }

  if (bytes_read < 0) {
    warn("custom_cp", "Read error");
  }

  if (verbose) {
    printf("%s -> %s\n", src, dest_path);
  }

  close(srcfd);
  close(destfd);
}

int main(int argc, char *argv[]) {
  int opt, interactive = 0, verbose = 0;

  while ((opt = getopt(argc, argv, "iv")) != -1) {
    switch (opt) {
    case 'i':
      interactive = 1;
      break;
    case 'v':
      verbose = 1;
      break;
    default:
      fprintf(stderr, "Usage: %s [-i] [-v] src dest\n", argv[0]);
      exit(EXIT_FAILURE);
    }
  }

  if (argc - optind != 2) {
    fprintf(stderr, "Usage: %s [-i] [-v] src dest\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  copy_file(argv[optind], argv[optind + 1], interactive, verbose);

  return 0;
}