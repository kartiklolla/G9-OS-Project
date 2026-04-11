/*
 * custom_grep â€” search for a literal pattern in files
 *
 * Usage: custom_grep [-n] [-i] pattern [file ...]
 *   -n  prefix each match with its line number
 *   -i  case-insensitive match (converts both sides to lowercase)
 *
 * No regex â€” plain substring search via strstr / case-folded variant.
 * Reads stdin when no file arguments are given.
 *
 * Syscalls used: open(2), read(2), close(2)
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include "common.h"

static int flag_linenum   = 0;
static int flag_icase     = 0;

/* Lowercase a string into dst (must be at least len+1 bytes). */
static void
to_lower(const char *src, char *dst, size_t len)
{
    for (size_t i = 0; i < len; i++)
        dst[i] = (char)tolower((unsigned char)src[i]);
    dst[len] = '\0';
}

static int
grep_file(FILE *fp, const char *filename, const char *pattern, int show_fname)
{
    size_t plen = strlen(pattern);
    char *lpat = NULL;
    if (flag_icase) {
        lpat = malloc(plen + 1);
        if (!lpat) { perror("malloc"); exit(1); }
        to_lower(pattern, lpat, plen);
    }

    char *line = NULL;
    size_t cap  = 0;
    ssize_t len;
    long lineno = 0;
    int matched = 0;

    while ((len = getline(&line, &cap, fp)) != -1) {
        lineno++;
        /* Strip trailing newline for display but keep the original length. */
        const char *haystack = line;

        int found = 0;
        if (flag_icase) {
            char *lline = malloc((size_t)len + 1);
            if (!lline) { perror("malloc"); exit(1); }
            to_lower(line, lline, (size_t)len);
            found = (strstr(lline, lpat) != NULL);
            free(lline);
        } else {
            found = (strstr(haystack, pattern) != NULL);
        }

        if (found) {
            matched = 1;
            if (show_fname)  printf("%s:", filename);
            if (flag_linenum) printf("%ld:", lineno);
            /* Print line (getline keeps the '\n'). */
            fwrite(line, 1, (size_t)len, stdout);
            /* Add newline if getline didn't (last line with no newline). */
            if (line[len - 1] != '\n') putchar('\n');
        }
    }

    free(line);
    free(lpat);
    return matched;
}

int
main(int argc, char *argv[])
{
    int i;
    for (i = 1; i < argc && argv[i][0] == '-' && argv[i][1] != '\0'; i++) {
        for (int j = 1; argv[i][j]; j++) {
            switch (argv[i][j]) {
            case 'n': flag_linenum = 1; break;
            case 'i': flag_icase   = 1; break;
            default:
                fprintf(stderr, "custom_grep: unknown option '-%c'\n", argv[i][j]);
                exit(1);
            }
        }
    }

    if (i >= argc) {
        fprintf(stderr, "Usage: custom_grep [-n] [-i] pattern [file ...]\n");
        exit(1);
    }

    const char *pattern = argv[i++];
    int any_match = 0;

    if (i == argc) {
        /* No files: read stdin. */
        any_match = grep_file(stdin, "stdin", pattern, 0);
    } else {
        int multiple = (argc - i) > 1;
        for (; i < argc; i++) {
            FILE *fp = fopen(argv[i], "r");
            if (!fp) {
                warn("custom_grep", argv[i]);
                continue;
            }
            if (grep_file(fp, argv[i], pattern, multiple))
                any_match = 1;
            fclose(fp);
        }
    }

    return any_match ? 0 : 1;
}