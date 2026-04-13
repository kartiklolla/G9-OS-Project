# Project 1 — xv6 Custom System Calls

**Group:** G9  
**Course:** Operating Systems  
**Kernel:** xv6-riscv (MIT)

---

## How to Build and Run

```bash
cd Project1_xv6_custom_sys_calls/xv6-riscv
make qemu          # build everything and launch QEMU
```

Exit QEMU at any time with **Ctrl-A x**.

To boot with a single CPU (useful for observing priority scheduling):

```bash
make CPUS=1 qemu
```

---

## System Calls Implemented

### 1. `getreadcount(void)` → `int`

Returns the total number of `read()` system calls made **system-wide since boot**.

**How it works:**  
A global integer `readcount` and a spinlock `readcount_lock` live in `kernel/proc.c`. Every time `sys_read` in `kernel/sysfile.c` is invoked, it calls `increment_readcount()` before returning. `getreadcount()` acquires the same lock and returns the current value. The lock prevents races on multi-CPU systems.

**Test:**
```
$ readcount
```
Expected output: count before, performs 3 reads via a pipe, count after (delta ≥ 3).

---

### 2. `getprocinfo(int pid, struct procinfo *p)` → `int`

Fills a caller-supplied struct with information about any live process by PID.

```c
struct procinfo {
  int    pid;
  int    ppid;
  int    state;   // 0=UNUSED 1=USED 2=SLEEPING 3=RUNNABLE 4=RUNNING 5=ZOMBIE
  uint64 sz;
  char   name[16];
};
```

**How it works:**  
`getprocinfo()` in `kernel/proc.c` walks the global `proc[]` table. For each entry it acquires `p->lock`, checks `p->pid`, copies fields into a local `struct procinfo`, releases the lock, then uses `copyout()` to safely write to the user pointer. Returns 0 on success, -1 if PID not found or `copyout` fails.

**Test:**
```
$ procinfo
```
Prints info for the current process, confirms -1 for a bogus PID, and queries a freshly forked child.

---

### 3. `setpriority(int pid, int priority)` → `int`  &  `getpriority(int pid)` → `int`

Add a user-settable **scheduling priority** to every process.

- Valid range: **1–20** (lower number = higher priority)
- Default: **10** (set in `allocproc`)
- `setpriority` returns 0 on success, -1 if PID not found or priority out of range
- `getpriority` returns the priority, or -1 if PID not found

**Scheduler change (`kernel/proc.c` — `scheduler()`):**  
The round-robin loop was replaced with a **two-pass priority scheduler**:
1. Pass 1 scans all RUNNABLE processes to find the lowest (best) priority value.
2. Pass 2 finds and runs the first RUNNABLE process with that priority value.

Each pass holds individual process locks only briefly — no two locks are held simultaneously, so there is no new deadlock risk.

**Test:**
```
$ priority
```
Verifies default value, set/get round-trip, out-of-range rejection, bogus PID rejection, and (on `CPUS=1`) that the high-priority child finishes before the low-priority one.

---

### 4. `wait_stat(int *wtime, int *rtime, int *status)` → `int`

Extended `wait()` that also reports how long the child spent in each state.

- `wtime` — ticks the child spent in **RUNNABLE** state (waiting to be scheduled)
- `rtime` — ticks the child spent in **RUNNING** state (on CPU)
- `status` — child exit status (same as regular `wait`)
- Returns child PID on success, -1 on error

**How it works:**  
Three fields (`ctime`, `rtime`, `stime`) were added to `struct proc` and initialised in `allocproc()`. `clockintr()` in `kernel/trap.c` calls `update_time_stats()` once per timer tick; that function walks the process table and increments the right counter for every live process. `kwait_stat()` behaves like `kwait()` but copies `rtime`, `stime`, and `xstate` out to the caller before calling `freeproc`.

**Test:**
```
$ waitstat
```
Forks a CPU-burning child (exit status 42), then prints rtime, wtime, and confirms the exit status.

---

### 5. `shm_open(int key, int size)` → `int`  &  `shm_close(int key)` → `int`

Minimal **shared-memory IPC** between processes.

- `shm_open` — creates or attaches to a shared region identified by `key`. `size` is rounded up to whole pages (max 4 pages / 16 KiB). Returns the user virtual address where the region is mapped, or -1 on error.
- `shm_close` — detaches the calling process. When the last process detaches, the physical pages are freed.

**How it works:**  
`kernel/shm.c` maintains a global table of 8 `struct shmregion` entries protected by a single spinlock. Each entry stores the key, number of pages, the physical addresses of those pages, and a reference count.  
On `shm_open`, physical pages are `kalloc`'d (once), zero-filled, and mapped into the process's address space at `p->sz` using `mappages`. Each process also has a small `shmattach[8]` array (in `struct proc`) tracking which keys it holds and at which virtual address.  
On `shm_close`, `uvmunmap` removes the mapping (without freeing physical memory). When refcount hits zero, `kfree` reclaims the pages.

**Test:**
```
$ shmtest
```
Parent writes `0xdeadbeef` into a shared page, child maps the same key and reads it back. Also tests double-open and close of unknown key.

---

## Syscall 6 — mutex_init / mutex_lock / mutex_unlock

Three system calls for user-space mutex locks (synchronization).

- `mutex_init(id)` — initializes a mutex slot (valid id: 0 to 15)
- `mutex_lock(id)` — acquires the lock, blocks if already held by another process
- `mutex_unlock(id)` — releases the lock and wakes up any waiting process

**Test:**
```
$ mutextest
```
**Files modified:** kernel/mutex.h, kernel/sysproc.c, kernel/syscall.h, kernel/syscall.c, kernel/defs.h, kernel/main.c, user/user.h, user/usys.pl, user/mutextest.c

---

## File Map

| Kernel file | Purpose |
|---|---|
| `kernel/syscall.h` | Syscall numbers 22–28 |
| `kernel/syscall.c` | Dispatch table entries |
| `kernel/sysproc.c` | Thin wrappers (`argint`/`argaddr` → implementation) |
| `kernel/defs.h` | Function declarations |
| `kernel/proc.h` | Extended `struct proc` (priority, ctime/rtime/stime, shm[]) |
| `kernel/proc.c` | `getreadcount`, `getprocinfo`, `setpriority`, `getpriority`, `update_time_stats`, `kwait_stat`, priority scheduler |
| `kernel/trap.c` | `clockintr` calls `update_time_stats` |
| `kernel/shm.h` | `struct shmregion`, `struct shmattach` |
| `kernel/shm.c` | `shminit`, `shm_open`, `shm_close` |
| `kernel/procinfo.h` | `struct procinfo` (shared with user space) |
| `kernel/mutex.h` | `struct umutex`, `MAX_MUTEXES` constant |
| `kernel/main.c` | Calls `shminit()` and `mutexinit()` at boot |

| User file | Purpose |
|---|---|
| `user/user.h` | User-space prototypes |
| `user/usys.pl` | Syscall stubs (RISC-V `ecall` wrappers) |
| `user/readcount.c` | Test for `getreadcount` |
| `user/procinfo.c` | Test for `getprocinfo` |
| `user/priority.c` | Test for `setpriority`/`getpriority` |
| `user/waitstat.c` | Test for `wait_stat` |
| `user/shmtest.c` | Test for `shm_open`/`shm_close` |
| `user/mutextest.c` | Test for `mutex_init`/`mutex_lock`/`mutex_unlock` |

