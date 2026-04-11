/*
 * custom_wc — count lines, words, and characters
 *
 * Usage: custom_wc [-l] [-w] [-c] [file ...]
 *   -l  print line count only
 *   -w  print word count only
 *   -c  print character (byte) count only
 *   No flags → print all three: lines words chars
 *
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

static int flag_lines = 0;
static int flag_words = 0;
static int flag_chars = 0;

typedef struct { long lines; long words; long chars; } Counts;

static Counts
count_fd(int fd)
{
    Counts c = {0, 0, 0};
    char buf[65536];
    ssize_t n;
    int in_word = 0;

    while ((n = read(fd, buf, sizeof(buf))) > 0) {
        for (ssize_t i = 0; i < n; i++) {
            c.chars++;
            if (buf[i] == '\n') c.lines++;
            if (isspace((unsigned char)buf[i])) {
                in_word = 0;
            } else if (!in_word) {
                in_word = 1;
                c.words++;
            }
        }
    }
    return c;
}

static void
print_counts(Counts c, const char *name, int show_all)
{
    if (show_all || flag_lines) printf("%8ld", c.lines);
    if (show_all || flag_words) printf("%8ld", c.words);
    if (show_all || flag_chars) printf("%8ld", c.chars);
    if (name) printf(" %s", name);
    printf("\n");
}

int
main(int argc, char *argv[])
{
    int i;
    for (i = 1; i < argc && argv[i][0] == '-' && argv[i][1] != '\0'; i++) {
        for (int j = 1; argv[i][j]; j++) {
            switch (argv[i][j]) {
            case 'l': flag_lines = 1; break;
            case 'w': flag_words = 1; break;
            case 'c': flag_chars = 1; break;
            default:
                fprintf(stderr, "custom_wc: unknown option '-%c'\n", argv[i][j]);
                exit(1);
            }
        }
    }

    int show_all = !(flag_lines || flag_words || flag_chars);

    if (i == argc) {
        Counts c = count_fd(STDIN_FILENO);
        print_counts(c, NULL, show_all);
        return 0;
    }

    Counts total = {0, 0, 0};
    int multiple = (argc - i) > 1;

    for (; i < argc; i++) {
        int fd = open(argv[i], O_RDONLY);
        if (fd < 0) { warn("custom_wc", argv[i]); continue; }
        Counts c = count_fd(fd);
        close(fd);
        print_counts(c, argv[i], show_all);
        total.lines += c.lines;
        total.words += c.words;
        total.chars += c.chars;
    }

    if (multiple)
        print_counts(total, "total", show_all);

    return 0;
}
