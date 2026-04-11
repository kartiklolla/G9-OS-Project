/*
 * custom_ls — list directory contents
 *
 * Usage: custom_ls [-l] [-a] [directory ...]
 *   -l  long listing (permissions, hard-link count, size, mtime, name)
 *   -a  include hidden entries (those starting with '.')
 *
 * Syscalls used: opendir(3), readdir(3), stat(2), localtime(3), strftime(3)
 */

#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE   500

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include "common.h"

static int flag_long = 0;
static int flag_all  = 0;

/* Build a 10-char permission string like "drwxr-xr-x". */
static void
mode_str(mode_t m, char *out)
{
    out[0] = S_ISDIR(m)  ? 'd' :
             S_ISLNK(m)  ? 'l' :
             S_ISCHR(m)  ? 'c' :
             S_ISBLK(m)  ? 'b' :
             S_ISFIFO(m) ? 'p' :
             S_ISSOCK(m) ? 's' : '-';

    out[1] = (m & S_IRUSR) ? 'r' : '-';
    out[2] = (m & S_IWUSR) ? 'w' : '-';
    out[3] = (m & S_IXUSR) ? 'x' : '-';
    out[4] = (m & S_IRGRP) ? 'r' : '-';
    out[5] = (m & S_IWGRP) ? 'w' : '-';
    out[6] = (m & S_IXGRP) ? 'x' : '-';
    out[7] = (m & S_IROTH) ? 'r' : '-';
    out[8] = (m & S_IWOTH) ? 'w' : '-';
    out[9] = (m & S_IXOTH) ? 'x' : '-';
    out[10] = '\0';
}

static void
print_entry(const char *dir, const char *name)
{
    if (!flag_long) {
        printf("%s\n", name);
        return;
    }

    /* Build full path for stat(). */
    char path[4096];
    snprintf(path, sizeof(path), "%s/%s", dir, name);

    struct stat st;
    if (lstat(path, &st) < 0) {
        warn("custom_ls", path);
        return;
    }

    char perms[11];
    mode_str(st.st_mode, perms);

    /* Format modification time. */
    struct tm *tm = localtime(&st.st_mtime);
    char timebuf[32];
    strftime(timebuf, sizeof(timebuf), "%b %e %H:%M", tm);

    printf("%s %2lu %8lld %s %s\n",
           perms,
           (unsigned long)st.st_nlink,
           (long long)st.st_size,
           timebuf,
           name);
}

static void
list_dir(const char *path)
{
    DIR *dp = opendir(path);
    if (!dp) {
        warn("custom_ls", path);
        return;
    }

    /* Collect entries into an array so we can sort them. */
    char **names = NULL;
    size_t count = 0, cap = 0;

    struct dirent *ent;
    while ((ent = readdir(dp)) != NULL) {
        if (!flag_all && ent->d_name[0] == '.')
            continue;

        if (count >= cap) {
            cap = cap ? cap * 2 : 32;
            names = realloc(names, cap * sizeof(char *));
            if (!names) { perror("realloc"); exit(1); }
        }
        names[count++] = strdup(ent->d_name);
    }
    closedir(dp);

    /* Simple alphabetical sort. */
    for (size_t i = 0; i < count; i++)
        for (size_t j = i + 1; j < count; j++)
            if (strcmp(names[i], names[j]) > 0) {
                char *tmp = names[i];
                names[i]  = names[j];
                names[j]  = tmp;
            }

    for (size_t i = 0; i < count; i++) {
        print_entry(path, names[i]);
        free(names[i]);
    }
    free(names);
}

int
main(int argc, char *argv[])
{
    int i;
    for (i = 1; i < argc && argv[i][0] == '-'; i++) {
        for (int j = 1; argv[i][j]; j++) {
            switch (argv[i][j]) {
            case 'l': flag_long = 1; break;
            case 'a': flag_all  = 1; break;
            default:
                fprintf(stderr, "custom_ls: unknown option '-%c'\n", argv[i][j]);
                exit(1);
            }
        }
    }

    if (i == argc) {
        list_dir(".");
    } else {
        int multiple = (argc - i) > 1;
        for (; i < argc; i++) {
            if (multiple) printf("%s:\n", argv[i]);
            list_dir(argv[i]);
            if (multiple && i + 1 < argc) printf("\n");
        }
    }
    return 0;
}
