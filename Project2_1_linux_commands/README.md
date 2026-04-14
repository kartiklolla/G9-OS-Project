# Project 2 — Custom UNIX Utilities + Shell

**Group:** G9  
**Language:** C11, POSIX syscalls only — no `system()`, no `popen()`, no shelling out.

---

## Build

```bash
cd Project2_1_linux_commands
make          # builds all binaries into bin/
make clean    # removes bin/
```

All binaries land in `bin/`. Add it to your PATH or run with `./bin/<name>`.

---

## Utilities

### `custom_ls` — list directory contents

```bash
custom_ls [OPTIONS] [directory ...]
  -l   long listing: permissions, link count, size, mtime, name
  -a   include hidden files (names starting with '.')
```

Examples:
```bash
./bin/custom_ls                  # list current directory
./bin/custom_ls -la /etc         # long listing including hidden files
./bin/custom_ls src bin          # list two directories
```

Syscalls: `opendir`, `readdir`, `lstat`, `localtime`, `strftime`

---

### `custom_cat` — concatenate files to stdout

```bash
custom_cat [OPTIONS] [file ...]
  -n   number all output lines
  -b   number non-empty lines only (overrides -n)
  -E   show '$' at end of each line
  -s   squeeze multiple adjacent blank lines into one
  -h   show help message
```

- With no arguments, reads from **stdin**.
- Use `-` explicitly to mean stdin among other files.
- Multiple files are concatenated in order with line numbers carried across files.
- Prints total lines processed to stderr on exit (when files are given).

Examples:
```bash
./bin/custom_cat file.txt             # print a file
./bin/custom_cat -n file.txt          # print with line numbers
./bin/custom_cat -b file.txt          # number non-empty lines only
./bin/custom_cat -E file.txt          # show $ at end of each line
./bin/custom_cat -s file.txt          # squeeze blank lines
./bin/custom_cat a.txt b.txt          # concatenate two files
echo "hello" | ./bin/custom_cat       # read from stdin
./bin/custom_cat a.txt - b.txt        # a.txt, then stdin, then b.txt
```

Syscalls: `fopen`, `fgetc`, `fclose`

---

### `custom_grep` — search for a pattern in files

```bash
custom_grep [OPTIONS] pattern [file ...]
  -n   prefix each matching line with its line number
  -i   case-insensitive match
```

- Plain substring match (no regular expressions).
- Reads stdin when no files are given.
- Exits 0 if any match found, 1 if no match.
- With multiple files, prefixes each match with the filename.

Examples:
```bash
./bin/custom_grep "error" app.log
./bin/custom_grep -n -i "warning" app.log
cat app.log | ./bin/custom_grep "timeout"
./bin/custom_grep -n "TODO" src/*.c
```

Syscalls: `fopen`, `getline`, `fclose`

---

### `custom_wc` — count lines, words, and characters

```bash
custom_wc [OPTIONS] [file ...]
  -l   print line count only
  -w   print word count only
  -c   print character (byte) count only
```

- No flags → prints all three: `lines  words  chars  filename`
- Multiple files → prints per-file counts and a `total` line.
- Reads stdin when no files given.

Examples:
```bash
./bin/custom_wc file.txt             # lines words chars
./bin/custom_wc -l *.c               # line count per file + total
echo "hello world" | ./bin/custom_wc
```

Syscalls: `open`, `read`, `close`

---

### `custom_cp` — copy files

```bash
custom_cp [-i] [-v] source dest
  -i   interactive: prompt before overwriting an existing destination
  -v   verbose: print "source -> dest" for each copy
```

- Destination can be a filename or an existing directory.
- When destination is a directory, the source basename is preserved.
- Copies file permission bits from source.
- Detects same-file copies via device/inode comparison.

Examples:
```bash
./bin/custom_cp foo.txt bar.txt          # copy to new name
./bin/custom_cp foo.txt /tmp/            # copy into directory
./bin/custom_cp -v foo.txt bar.txt       # verbose output
./bin/custom_cp -i foo.txt bar.txt       # prompt before overwrite
```

Syscalls: `open`, `read`, `write`, `close`, `stat`, `fstat`

---

### `custom_mv` — move or rename files

```bash
custom_mv [-i] [-v] source dest
  -i   interactive: prompt before overwriting an existing destination
  -v   verbose: print "source -> dest" after move
```

- Uses `rename(2)` when source and destination are on the same filesystem.
- Falls back to **copy + unlink** when `rename` returns `EXDEV` (cross-device).
- When destination is a directory, preserves source basename.
- Detects same-file moves via device/inode comparison.

Examples:
```bash
./bin/custom_mv old.txt new.txt          # rename
./bin/custom_mv report.txt /tmp/         # move into directory
./bin/custom_mv -v a.txt b.txt           # verbose
./bin/custom_mv -i a.txt b.txt           # prompt before overwrite
```

Syscalls: `rename`, `open`, `read`, `write`, `close`, `unlink`, `stat`

---

### `custom_rm` — remove files and directories

```bash
custom_rm [-r] file ...
  -r / -R   recursively remove directories and their contents
```

- Without `-r`, refuses to remove directories (prints an error).
- With `-r`, depth-first removal: files first, then the directory itself.
- Does not follow symbolic links (uses `lstat`) — safe behavior.

Examples:
```bash
./bin/custom_rm file.txt
./bin/custom_rm -r old_project/
./bin/custom_rm a.txt b.txt c.txt
```

Syscalls: `unlink`, `rmdir`, `opendir`, `readdir`, `lstat`

---

### `custom_shell` — interactive and scripted shell

```bash
./bin/custom_shell               # interactive mode
./bin/custom_shell < script.sh   # run a script
echo "ls | wc -l" | ./bin/custom_shell
```

**Auto-PATH:** on startup the shell reads its own executable path via `/proc/self/exe`, extracts the directory, and prepends it to `$PATH`. This means all `custom_*` commands are available by name inside the shell without any `./bin/` prefix.

**Built-in commands:**

| Command | Description |
|---|---|
| `cd [dir]` | Change directory (`$HOME` if no argument) |
| `pwd` | Print working directory |
| `exit [code]` | Exit shell with optional status code |

**External commands:** any program on `$PATH` via `fork` + `execvp`.

**Features:**

| Feature | Syntax | Example |
|---|---|---|
| Pipes | `cmd1 \| cmd2` | `ls \| wc -l` |
| Output redirect | `cmd > file` | `ls > out.txt` |
| Append redirect | `cmd >> file` | `echo hi >> log.txt` |
| Input redirect | `cmd < file` | `wc -l < data.txt` |
| Background | `cmd &` | `sleep 5 &` |

**Demo session:**
```
/home/user $ pwd
/home/user
/home/user $ cd /tmp
/tmp $ echo "hello world" > test.txt
/tmp $ custom_cat test.txt
hello world
/tmp $ custom_cat test.txt | custom_wc -w
2
/tmp $ custom_ls -l | custom_grep test
-rw-r--r-- 1 ... test.txt
/tmp $ sleep 2 &
[bg] pid 12345
/tmp $ exit
```

Syscalls: `fork`, `execvp`, `waitpid`, `pipe`, `dup2`, `open`, `chdir`, `getcwd`, `readlink`, `setenv`

---

## Known Limitations

- `custom_ls -l` shows owner/group as numeric IDs (uid/gid) — resolving to usernames would require `getpwuid`/`getgrgid`.
- `custom_cat` always prints total lines to stderr when processing files; this goes to stderr so it does not pollute piped output.
- `custom_cp` and `custom_mv` accept exactly two positional arguments (`src dest`); moving multiple files into a directory is not supported.
- `custom_shell` does not support command substitution (`$()`), variable expansion (`$VAR`), or here-docs.
- `custom_shell` tokenizes on whitespace only — quoted strings with spaces are not handled.
- `custom_grep` uses literal substring matching — no regular expressions.
- `custom_rm -r` does not follow symbolic links (uses `lstat`), which is the correct safe behavior.
