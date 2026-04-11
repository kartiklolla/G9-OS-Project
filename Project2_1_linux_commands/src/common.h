#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* Print "progname: message: strerror" to stderr and exit. */
static inline void
die(const char *prog, const char *msg)
{
    fprintf(stderr, "%s: %s: %s\n", prog, msg, strerror(errno));
    exit(1);
}

/* Print "progname: message" to stderr and exit (no errno). */
static inline void
die_msg(const char *prog, const char *msg)
{
    fprintf(stderr, "%s: %s\n", prog, msg);
    exit(1);
}

/* Print "progname: fmt ..." to stderr (no exit). */
static inline void
warn(const char *prog, const char *msg)
{
    fprintf(stderr, "%s: %s: %s\n", prog, msg, strerror(errno));
}

#endif /* COMMON_H */
