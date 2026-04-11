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

// Fallback copy function if rename fails across file systems (EXDEV)
static int copy_and_unlink(const char *src, const char *dest) {
  struct stat st_src;
  if (stat(src, &st_src) != 0) return -1;

  int srcfd = open(src, O_RDONLY);
  if (srcfd < 0) return -1;

  int destfd = open(dest, O_WRONLY | O_CREAT | O_TRUNC, st_src.st_mode);
  if (destfd < 0) {
    close(srcfd);
    return -1;
  }

  char buf[BUF_SIZE];
  ssize_t bytes_read;

  while ((bytes_read = read(srcfd, buf, BUF_SIZE)) > 0) {
    ssize_t bytes_written = 0;
    while (bytes_written < bytes_read) {
      ssize_t w = write(destfd, buf + bytes_written, bytes_read - bytes_written);
      if (w < 0) {
        close(srcfd);
        close(destfd);
        unlink(dest);
        return -1;
      }
      bytes_written += w;
    }
  }

  close(srcfd);
  close(destfd);

  if (bytes_read < 0) {
    unlink(dest);
    return -1;
  }

  unlink(src);
  return 0;
}

void move_file(const char *src, const char *dest, int interactive,
               int verbose) {
  struct stat st_src, st_dest;

  if (stat(src, &st_src) != 0) {
    warn("custom_mv", "Source stat failed");
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

      if (stat(dest_path, &st_dest) == 0) {
        if (st_src.st_dev == st_dest.st_dev && st_src.st_ino == st_dest.st_ino) {
          fprintf(stderr, "custom_mv: '%s' and '%s' are the same file\n", src, dest_path);
          return;
        }
      }
    } else {
      if (st_src.st_dev == st_dest.st_dev && st_src.st_ino == st_dest.st_ino) {
        fprintf(stderr, "custom_mv: '%s' and '%s' are the same file\n", src, dest_path);
        return;
      }
    }
  }

  //-i flag checks if destination exists or not
  if (interactive && access(dest_path, F_OK) == 0) {
    printf("custom_mv: Overwrite '%s'? (y/n): ", dest_path);
    char choice = getchar();
    if (choice != 'y' && choice != 'Y') {
      return;
    }
  }

  if (rename(src, dest_path) != 0) {
    if (errno == EXDEV) {
      // cross-device link, fallback to copy + delete
      if (copy_and_unlink(src, dest_path) != 0) {
        warn("custom_mv", "Failed to move file across mounts");
        return;
      }
    } else {
      warn("custom_mv", "Rename error");
      return;
    }
  }

  if (verbose) {
    printf("%s -> %s\n", src, dest_path);
  }
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

  move_file(argv[optind], argv[optind + 1], interactive, verbose);

  return 0;
}
