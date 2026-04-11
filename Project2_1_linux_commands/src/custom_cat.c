// custom_cat.c
// My implementation of the cat command for Project 2
// What it does: reads files and prints them to the screen
// If no file is given, it reads whatever you type (stdin)
// I also added flag support like -n (line numbers), -b (skip blank lines), -E (show line endings)


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

#define BUFSIZE 4096

// flags for options
int show_line_numbers = 0;     // -n flag
int number_nonempty   = 0;     // -b flag
int show_dollar       = 0;     // -E flag (show $ at end of line)
int squeeze           = 0;     // -s flag (remove extra blank lines)

// global line counter, stays across files
int lineno = 1;

// this function do the actual printing
// i read char by char, so i can handle the flags properly
void print_file(FILE *fp, const char *filename) {

    int ch;
    int at_line_start = 1;   //  starting of a new line
    int blank_count   = 0;   // for squeeze flag

    while ((ch = fgetc(fp)) != EOF) {

        // -s: if we see more than one blank line in a row, skip
        if (ch == '\n') {
            // check if previous char was also newline (blank line)
            if (at_line_start == 1) {
                blank_count++;
                if (squeeze && blank_count > 1) {
                    continue;  // skip extra blank lines
                }
            } else {
                blank_count = 0;
            }

            // -E: print $ before the newline
            if (show_dollar) {
                putchar('$');
            }
            putchar('\n');
            at_line_start = 1;

        } else {
            // we have a real character,this is not a blank line
            blank_count = 0;

            // print line number at start of each line
            if (at_line_start) {
                if (number_nonempty) {
                    // -b: only number non-blank lines
                    printf("%6d\t", lineno++);
                } else if (show_line_numbers) {
                    // -n: number every line
                    printf("%6d\t", lineno++);
                }
                at_line_start = 0;
            }

            putchar(ch);
        }
    }

    // check if there was a read error
    if (ferror(fp)) {
        fprintf(stderr, "custom_cat: error reading %s: %s\n", filename, strerror(errno));
    }
}

void show_usage() {
    printf("Usage: custom_cat [OPTIONS] [FILE...]\n");
    printf("Options:\n");
    printf("  -n   number all lines\n");
    printf("  -b   number non-empty lines only\n");
    printf("  -E   show $ at end of each line\n");
    printf("  -s   squeeze multiple blank lines\n");
    printf("  -h   show this help\n");
    printf("If no file given, reads from stdin\n");
}

int main(int argc, char *argv[]) {

    int i;
    int file_start = 1;  // index where filenames start in argv

    // parse the flags first
    for (i = 1; i < argc; i++) {
        if (argv[i][0] != '-' || strcmp(argv[i], "-") == 0) {
            break;  // not a flag, must be a filename
        }

        // go through each char in the flag string e.g. -nb
        int j;
        for (j = 1; argv[i][j] != '\0'; j++) {
            char c = argv[i][j];
            if (c == 'n') {
                show_line_numbers = 1;
            } else if (c == 'b') {
                number_nonempty = 1;
            } else if (c == 'E') {
                show_dollar = 1;
            } else if (c == 's') {
                squeeze = 1;
            } else if (c == 'h') {
                show_usage();
                return 0;
            } else {
                fprintf(stderr, "custom_cat: unknown option -%c\n", c);
                fprintf(stderr, "Try 'custom_cat -h' for help\n");
                return 1;
            }
        }
        file_start++;
    }

    // if -b is set, ignore -n (same as real cat behaviour)
    if (number_nonempty) {
        show_line_numbers = 0;
    }

    // no files given, read from keyboard / stdin
    if (file_start >= argc) {
        print_file(stdin, "stdin");
        return 0;
    }

    // loop through all the files given
    int exit_status = 0;
    for (i = file_start; i < argc; i++) {

        // "-" means read from stdin
        if (strcmp(argv[i], "-") == 0) {
            print_file(stdin, "stdin");
            continue;
        }

        FILE *fp = fopen(argv[i], "r");
        if (fp == NULL) {
            // print error but keep going with other files
            fprintf(stderr, "custom_cat: cannot open '%s': %s\n", argv[i], strerror(errno));
            exit_status = 1;
            continue;
        }

        print_file(fp, argv[i]);
        fclose(fp);
    }

    return exit_status;
}
