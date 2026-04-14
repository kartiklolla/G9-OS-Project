# G9 — Operating Systems Course Project

**Group:** G9 | **Course:** Operating Systems

This repository contains two independent projects completed as part of the OS course.

---

## Project 1 — xv6 Custom System Calls

**Location:** `Project1_xv6_custom_sys_calls/`

Five new system calls added to the MIT xv6-riscv kernel. Each syscall is wired through all nine pipeline steps: syscall number, dispatch table, kernel wrapper, implementation, user-space stub, and a test program.

| Syscall | What it does |
|---|---|
| `getreadcount()` | Returns total `read()` calls made system-wide since boot |
| `getprocinfo(pid, *info)` | Fills a struct with pid, ppid, state, size, name of any process |
| `setpriority(pid, pri)` / `getpriority(pid)` | Per-process priority (1–20); scheduler picks lowest-number RUNNABLE process |
| `wait_stat(*wtime, *rtime, *status)` | Extended `wait()` reporting ticks child spent running and waiting |
| `shm_open(key, size)` / `shm_close(key)` | Shared-memory IPC: map the same physical pages into multiple processes |

**Build & run:**
```bash
cd Project1_xv6_custom_sys_calls/xv6-riscv
make qemu          # boots xv6 in QEMU
# run test programs: readcount, procinfo, priority, waitstat, shmtest
# exit QEMU: Ctrl-A x
```

See [`Project1_xv6_custom_sys_calls/README.md`](Project1_xv6_custom_sys_calls/README.md) for full details.

---

## Project 2 — Custom UNIX Utilities + Shell

**Location:** `Project2_1_linux_commands/`

Eight POSIX utilities written from scratch in C11. No `system()`, no `popen()` — only direct syscalls.

| Utility | Description |
|---|---|
| `custom_ls` | List directory; `-l` long format, `-a` show hidden |
| `custom_cat` | Concatenate files to stdout; reads stdin when no args |
| `custom_grep` | Literal substring search; `-n` line numbers, `-i` case-insensitive |
| `custom_wc` | Count lines/words/chars; `-l`/`-w`/`-c` selectors |
| `custom_cp` | Copy files; handles destination directory |
| `custom_mv` | Move/rename; `rename(2)` with cross-device fallback |
| `custom_rm` | Remove files; `-r` recursive directory removal |
| `custom_shell` | Interactive shell with pipes, redirections (`<` `>` `>>`), background (`&`); auto-adds `bin/` to `$PATH` so all `custom_*` commands work by name inside the shell |

**Build & run:**
```bash
cd Project2_1_linux_commands
make                    # builds all binaries into bin/
./bin/custom_shell      # launch the shell
make clean
```

See [`Project2_1_linux_commands/README.md`](Project2_1_linux_commands/README.md) for usage examples.

---

## Repository Layout

```
G9-OS-Project/
├── README.md                                  ← this file
├── Project1_xv6_custom_sys_calls/
│   ├── README.md                              ← build & syscall guide
│   ├── documentation.md                       ← technical implementation notes
│   └── xv6-riscv/
│       ├── kernel/   (modified kernel source)
│       └── user/     (test programs)
└── Project2_1_linux_commands/
    ├── README.md                              ← usage examples
    ├── documentation.md                       ← design notes & syscall table
    ├── Makefile
    └── src/          (C source for all utilities)
```
