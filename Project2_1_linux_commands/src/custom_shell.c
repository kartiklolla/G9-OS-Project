/*
 * custom_shell — a minimal UNIX shell
 *
 * Features:
 *   Built-ins : cd [dir], pwd, exit [code]
 *   External  : fork + execvp + waitpid
 *   Pipes     : cmd1 | cmd2 | cmd3 ...
 *   Redirect  : cmd > file   (stdout to file, truncate)
 *               cmd >> file  (stdout to file, append)
 *               cmd < file   (stdin from file)
 *   Background: cmd &        (run without waiting)
 *
 * Syscalls used: fork(2), execvp(3), waitpid(2), pipe(2), open(2),
 *                dup2(2), close(2), chdir(2), getcwd(3), read(2), write(2)
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include "common.h"

#define MAX_ARGS    128
#define MAX_CMDS    32      /* max segments in a pipeline */
#define MAX_LINE    4096

/* ------------------------------------------------------------------ */
/* Tokenizer                                                            */
/* ------------------------------------------------------------------ */

/* Split line into an array of token pointers (in-place, modifies line).
   Returns number of tokens. Handles simple quoting for spaces. */
static int
tokenize(char *line, char **toks, int max)
{
    int n = 0;
    char *p = line;

    while (*p) {
        /* Skip whitespace. */
        while (*p && isspace((unsigned char)*p)) p++;
        if (!*p) break;
        if (n >= max - 1) break;

        toks[n++] = p;

        /* Advance to end of token (spaces or special chars terminate). */
        while (*p && !isspace((unsigned char)*p)) p++;
        if (*p) { *p = '\0'; p++; }
    }
    toks[n] = NULL;
    return n;
}

/* ------------------------------------------------------------------ */
/* Command structure                                                    */
/* ------------------------------------------------------------------ */

typedef struct {
    char *argv[MAX_ARGS];  /* NULL-terminated argument vector */
    int   argc;
    char *redir_in;        /* < filename, or NULL */
    char *redir_out;       /* > filename, or NULL */
    int   append_out;      /* 1 if >>, 0 if > */
} Cmd;

/* Parse tokens into an array of Cmd structs separated by '|'.
   Returns number of commands, or -1 on parse error.
   Sets *background = 1 if trailing '&' found. */
static int
parse_pipeline(char **toks, int ntoks, Cmd *cmds, int *background)
{
    *background = 0;
    int ncmds = 0;
    Cmd *cur = &cmds[ncmds++];
    memset(cur, 0, sizeof(*cur));

    for (int i = 0; i < ntoks; i++) {
        char *t = toks[i];

        if (strcmp(t, "|") == 0) {
            cur->argv[cur->argc] = NULL;
            if (ncmds >= MAX_CMDS) {
                fprintf(stderr, "custom_shell: too many pipes\n");
                return -1;
            }
            cur = &cmds[ncmds++];
            memset(cur, 0, sizeof(*cur));

        } else if (strcmp(t, "<") == 0) {
            if (i + 1 >= ntoks) { fprintf(stderr, "custom_shell: missing file after '<'\n"); return -1; }
            cur->redir_in = toks[++i];

        } else if (strcmp(t, ">") == 0) {
            if (i + 1 >= ntoks) { fprintf(stderr, "custom_shell: missing file after '>'\n"); return -1; }
            cur->redir_out  = toks[++i];
            cur->append_out = 0;

        } else if (strcmp(t, ">>") == 0) {
            if (i + 1 >= ntoks) { fprintf(stderr, "custom_shell: missing file after '>>'\n"); return -1; }
            cur->redir_out  = toks[++i];
            cur->append_out = 1;

        } else if (strcmp(t, "&") == 0 && i == ntoks - 1) {
            *background = 1;

        } else {
            if (cur->argc >= MAX_ARGS - 1) {
                fprintf(stderr, "custom_shell: too many arguments\n");
                return -1;
            }
            cur->argv[cur->argc++] = t;
        }
    }
    cur->argv[cur->argc] = NULL;

    /* Drop empty last command (trailing pipe). */
    if (ncmds > 0 && cmds[ncmds - 1].argc == 0) ncmds--;
    return ncmds;
}

/* ------------------------------------------------------------------ */
/* Built-in commands                                                    */
/* ------------------------------------------------------------------ */

/* Returns 1 if cmd was a built-in and was handled, 0 otherwise. */
static int
run_builtin(Cmd *cmd, int *last_status)
{
    if (cmd->argc == 0) return 0;

    if (strcmp(cmd->argv[0], "exit") == 0) {
        int code = (cmd->argc > 1) ? atoi(cmd->argv[1]) : 0;
        exit(code);
    }

    if (strcmp(cmd->argv[0], "cd") == 0) {
        const char *dir = (cmd->argc > 1) ? cmd->argv[1] : getenv("HOME");
        if (!dir) dir = "/";
        if (chdir(dir) < 0) {
            fprintf(stderr, "custom_shell: cd: %s: %s\n", dir, strerror(errno));
            *last_status = 1;
        } else {
            *last_status = 0;
        }
        return 1;
    }

    if (strcmp(cmd->argv[0], "pwd") == 0) {
        char cwd[4096];
        if (getcwd(cwd, sizeof(cwd)) == NULL) {
            perror("custom_shell: pwd");
            *last_status = 1;
        } else {
            printf("%s\n", cwd);
            *last_status = 0;
        }
        return 1;
    }

    return 0;
}

/* ------------------------------------------------------------------ */
/* Pipeline execution                                                   */
/* ------------------------------------------------------------------ */

/* Apply redirections for one command in a child process. */
static void
apply_redirections(Cmd *cmd)
{
    if (cmd->redir_in) {
        int fd = open(cmd->redir_in, O_RDONLY);
        if (fd < 0) { perror(cmd->redir_in); exit(1); }
        dup2(fd, STDIN_FILENO);
        close(fd);
    }
    if (cmd->redir_out) {
        int flags = O_WRONLY | O_CREAT | (cmd->append_out ? O_APPEND : O_TRUNC);
        int fd = open(cmd->redir_out, flags, 0644);
        if (fd < 0) { perror(cmd->redir_out); exit(1); }
        dup2(fd, STDOUT_FILENO);
        close(fd);
    }
}

static int
run_pipeline(Cmd *cmds, int ncmds, int background)
{
    if (ncmds == 0) return 0;

    /* Single built-in with no pipe. */
    if (ncmds == 1) {
        int status = 0;
        if (run_builtin(&cmds[0], &status)) return status;
    }

    /* Create ncmds-1 pipes. */
    int pipes[MAX_CMDS - 1][2];
    for (int i = 0; i < ncmds - 1; i++) {
        if (pipe(pipes[i]) < 0) { perror("pipe"); return 1; }
    }

    pid_t pids[MAX_CMDS];

    for (int i = 0; i < ncmds; i++) {
        /* Built-ins in a pipeline run as external (their own fork). */
        pid_t pid = fork();
        if (pid < 0) { perror("fork"); return 1; }

        if (pid == 0) {
            /* Child: wire up pipe ends. */
            if (i > 0)         dup2(pipes[i - 1][0], STDIN_FILENO);
            if (i < ncmds - 1) dup2(pipes[i][1],     STDOUT_FILENO);

            /* Close all pipe fds in child. */
            for (int k = 0; k < ncmds - 1; k++) {
                close(pipes[k][0]);
                close(pipes[k][1]);
            }

            apply_redirections(&cmds[i]);

            if (cmds[i].argc == 0) exit(0);
            execvp(cmds[i].argv[0], cmds[i].argv);
            fprintf(stderr, "custom_shell: %s: %s\n",
                    cmds[i].argv[0], strerror(errno));
            exit(127);
        }
        pids[i] = pid;
    }

    /* Parent: close all pipe fds. */
    for (int i = 0; i < ncmds - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    /* Wait. */
    int last_status = 0;
    if (!background) {
        for (int i = 0; i < ncmds; i++) {
            int ws;
            waitpid(pids[i], &ws, 0);
            if (i == ncmds - 1)
                last_status = WIFEXITED(ws) ? WEXITSTATUS(ws) : 1;
        }
    } else {
        printf("[bg] pid %d\n", pids[ncmds - 1]);
    }
    return last_status;
}

/* ------------------------------------------------------------------ */
/* REPL                                                                 */
/* ------------------------------------------------------------------ */

static void
print_prompt(void)
{
    char cwd[4096];
    if (getcwd(cwd, sizeof(cwd)) != NULL)
        fprintf(stderr, "%s $ ", cwd);
    else
        fprintf(stderr, "$ ");
}

int
main(void)
{
    char line[MAX_LINE];
    int  last_status = 0;
    int  interactive = isatty(STDIN_FILENO);

    while (1) {
        /* Reap any finished background children without blocking. */
        while (waitpid(-1, NULL, WNOHANG) > 0)
            ;

        if (interactive) print_prompt();

        if (!fgets(line, sizeof(line), stdin)) {
            if (interactive) putchar('\n');
            break;  /* EOF */
        }

        /* Strip trailing newline. */
        line[strcspn(line, "\n")] = '\0';

        /* Skip blank / comment lines. */
        char *p = line;
        while (isspace((unsigned char)*p)) p++;
        if (*p == '\0' || *p == '#') continue;

        char *toks[MAX_ARGS * MAX_CMDS + 1];
        int ntoks = tokenize(p, toks, (int)(sizeof(toks)/sizeof(toks[0])));
        if (ntoks == 0) continue;

        Cmd cmds[MAX_CMDS];
        int background = 0;
        int ncmds = parse_pipeline(toks, ntoks, cmds, &background);
        if (ncmds <= 0) continue;

        last_status = run_pipeline(cmds, ncmds, background);
        (void)last_status;
    }

    return last_status;
}
