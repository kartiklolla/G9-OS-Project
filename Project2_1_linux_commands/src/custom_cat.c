// custom_cat.c
// Name: Kandlavath Mohan Naik
// Roll No: 24JE0631
// My implementation of the cat command for Project 2
// What it does: reads files and prints them to the screen
// If no file is given, it reads whatever you type (stdin)
// I also added flag support like -n, -b, -E, -s

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "common.h"

int show_line_numbers = 0;  // -n flag
int skip_empty = 0;         // -b flag
int end_marker = 0;         // -E flag
int skip_blanks = 0;        // -s flag
int my_line_cnt = 1;        // line counter across files

// fgetc reads one char at a time so i can apply flags on each line
void print_file(FILE *fp, const char *filename) {
    int ch;
    int new_line = 1;
    int empty_lines = 0;

    while ((ch = fgetc(fp)) != EOF) {
        if (ch == '\n') {
            if (new_line == 1) {
                empty_lines++;
                if (skip_blanks && empty_lines > 1) continue; // -s: skip extra blank lines
                if (show_line_numbers && !skip_empty)
                    printf("%6d\t", my_line_cnt++); // -n: number blank lines too
            } else {
                empty_lines = 0;
            }
            if (end_marker) putchar('$'); // -E: print $ before newline
            putchar('\n');
            new_line = 1;
        } else {
            empty_lines = 0;
            if (new_line) {
                if (skip_empty || show_line_numbers)
                    printf("%6d\t", my_line_cnt++); // print line number
                new_line = 0;
            }
            putchar(ch);
        }
    }
    if (ferror(fp))
        fprintf(stderr, "custom_cat: error reading %s: %s\n", filename, strerror(errno));
}

void show_usage() {
    printf("=== custom_cat by Mohan ===\n");
    printf("Usage: custom_cat [OPTIONS] [FILE...]\n");
    printf("  -n   number all lines\n");
    printf("  -b   number non-empty lines only\n");
    printf("  -E   show $ at end of each line\n");
    printf("  -s   squeeze multiple blank lines\n");
    printf("  -h   show this help\n");
    printf("If no file given, reads from stdin\n");
}

int main(int argc, char *argv[]) {
    int i;
    int file_start = 1;

    // parse flags from arguments
    for (i = 1; i < argc; i++) {
        if (argv[i][0] != '-' || strcmp(argv[i], "-") == 0) break;
        int j;
        for (j = 1; argv[i][j] != '\0'; j++) {
            char c = argv[i][j];
            if (c == 'n') show_line_numbers = 1;
            else if (c == 'b') skip_empty = 1;
            else if (c == 'E') end_marker = 1;
            else if (c == 's') skip_blanks = 1;
            else if (c == 'h') { show_usage(); return 0; }
            else { fprintf(stderr, "unknown option -%c\n", c); return 1; }
        }
        file_start++;
    }

    if (skip_empty) show_line_numbers = 0; // -b overrides -n

    // no file given, read from stdin
    if (file_start >= argc) { print_file(stdin, "stdin"); return 0; }

    int exit_status = 0;
    for (i = file_start; i < argc; i++) {
        if (strcmp(argv[i], "-") == 0) { print_file(stdin, "stdin"); continue; }
        FILE *fp = fopen(argv[i], "r");
        if (fp == NULL) {
            fprintf(stderr, "cannot open '%s': %s\n", argv[i], strerror(errno));
            exit_status = 1;
            continue;
        }
        print_file(fp, argv[i]);
        fclose(fp);
    }

    // print total lines at the end
    fprintf(stderr, "total lines processed: %d\n", my_line_cnt - 1);
    return exit_status;
}
