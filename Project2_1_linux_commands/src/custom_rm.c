/*
 * custom_rm — remove files and directories
 *
 * Usage: custom_rm [-r] file ...
 *   -r / -R   recursively remove directories and their contents
 *
 * Without -r, refuses to remove directories.
 * With -r, performs depth-first removal using opendir/readdir/rmdir/unlink.
 *
 * Syscalls used: unlink(2), rmdir(2), opendir(3), readdir(3), lstat(2)
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "common.h"

static int flag_recursive = 0;

static int rm_path(const char *path);

static int
rm_dir_recursive(const char *path)
{
    DIR *dp = opendir(path);
    if (!dp) { warn("custom_rm", path); return -1; }

    struct dirent *ent;
    int err = 0;

    while ((ent = readdir(dp)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
            continue;
        char child[4096];
        snprintf(child, sizeof(child), "%s/%s", path, ent->d_name);
        if (rm_path(child) < 0) err = 1;
    }
    closedir(dp);

    if (rmdir(path) < 0) { warn("custom_rm", path); return -1; }
    return err ? -1 : 0;
}

static int
rm_path(const char *path)
{
    struct stat st;
    if (lstat(path, &st) < 0) { warn("custom_rm", path); return -1; }

    if (S_ISDIR(st.st_mode)) {
        if (!flag_recursive) {
            fprintf(stderr, "custom_rm: '%s' is a directory (use -r)\n", path);
            return -1;
        }
        return rm_dir_recursive(path);
    }

    if (unlink(path) < 0) { warn("custom_rm", path); return -1; }
    printf("Deleted: %s\n", path);
    return 0;
}

int
main(int argc, char *argv[])
{
    int i;
    for (i = 1; i < argc && argv[i][0] == '-' && argv[i][1] != '\0'; i++) {
        for (int j = 1; argv[i][j]; j++) {
            switch (argv[i][j]) {
            case 'r': case 'R': flag_recursive = 1; break;
            default:
                fprintf(stderr, "custom_rm: unknown option '-%c'\n", argv[i][j]);
                exit(1);
            }
        }
    }

    if (i == argc) {
        fprintf(stderr, "Usage: custom_rm [-r] file ...\n");
        exit(1);
    }

    int ret = 0;
    for (; i < argc; i++)
        if (rm_path(argv[i]) < 0) ret = 1;
    return ret;
}
