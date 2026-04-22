# Linux IPC — Study Notes

## Key Takeaways

### Performance Hierarchy (fastest to slowest for data transfer)
1. **Shared Memory** — Direct memory access, no kernel involvement after setup
2. **Pipe** — Kernel buffer copy, but very optimized in the kernel
3. **Message Queue** — Kernel copy with priority sorting overhead
4. **Unix Domain Socket** — Kernel copy, but no network stack
5. **TCP/UDP Socket** — Full network stack processing

### Common Pitfalls
- **Shared memory without synchronization** — Race conditions are guaranteed. Always pair with semaphore or mutex.
- **Not closing unused pipe ends** — Reader never gets EOF; writer gets SIGPIPE.
- **Forgetting `mq_unlink()`/`shm_unlink()`** — IPC objects persist across process restarts.
- **Using `printf()` in signal handlers** — Not async-signal-safe. Use `write()` instead.
- **Signal handler race conditions** — Use `volatile sig_atomic_t` for flags.
- **Deadlock with semaphores** — Always acquire in consistent order; use timeouts.

### POSIX vs System V IPC
This repository uses POSIX IPC throughout. POSIX is the modern standard:
- Cleaner API (file-descriptor-based)
- Better integration with the filesystem (`/dev/shm/`, `/dev/mqueue/`)
- More portable across Unix-like systems

System V IPC (`shmget`, `semget`, `msgget`) is legacy but still found in older codebases.

### Further Reading
- Chapter 43-57 of "The Linux Programming Interface" by Michael Kerrisk
- `man 7 pipe`, `man 7 mq_overview`, `man 7 shm_overview`, `man 7 sem_overview`
- Linux kernel source: `fs/pipe.c`, `ipc/mqueue.c`, `mm/shmem.c`
