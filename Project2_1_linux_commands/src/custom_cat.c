// custom_cat.c
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
int number_nonempty = 0;    // -b flag
int show_dollar = 0;        // -E flag
int squeeze = 0;            // -s flag
int lineno = 1;             // line counter across files

// read char by char so we can handle flags properly
void print_file(FILE *fp, const char *filename) {
    int ch;
    int at_line_start = 1;
    int blank_count = 0;

    while ((ch = fgetc(fp)) != EOF) {
        if (ch == '\n') {
            if (at_line_start == 1) {
                blank_count++;
                if (squeeze && blank_count > 1) continue; // skip extra blank lines
                if (show_line_numbers && !number_nonempty)
                    printf("%6d\t", lineno++); // number blank lines too for -n
            } else {
                blank_count = 0;
            }
            if (show_dollar) putchar('$'); // print $ before newline for -E
            putchar('\n');
            at_line_start = 1;
        } else {
            blank_count = 0;
            if (at_line_start) {
                if (number_nonempty || show_line_numbers)
                    printf("%6d\t", lineno++); // print line number
                at_line_start = 0;
            }
            putchar(ch);
        }
    }
    if (ferror(fp))
        fprintf(stderr, "custom_cat: error reading %s: %s\n", filename, strerror(errno));
}

void show_usage() {
    printf("Usage: custom_cat [OPTIONS] [FILE...]\n");
    printf("  -n   number all lines\n");
    printf("  -b   number non-empty lines only\n");
    printf("  -E   show $ at end of each line\n");
    printf("  -s   squeeze multiple blank lines\n");
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
            else if (c == 'b') number_nonempty = 1;
            else if (c == 'E') show_dollar = 1;
            else if (c == 's') squeeze = 1;
            else if (c == 'h') { show_usage(); return 0; }
            else { fprintf(stderr, "unknown option -%c\n", c); return 1; }
        }
        file_start++;
    }

    if (number_nonempty) show_line_numbers = 0; // -b overrides -n

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
    return exit_status;
}
