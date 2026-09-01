# Linux IPC — Study Notes

## Key Takeaways

### Performance Notes (data transfer, same host)

Rough tiers rather than a strict ranking — real numbers depend on payload size,
batching, and kernel version, so always measure for your own workload.

1. **Shared Memory** — fastest: after `mmap()`, reads/writes touch memory
   directly and the kernel copies nothing per operation.
2. **Pipe / FIFO and UNIX domain socket** — one kernel copy each way, no network
   stack. In practice these are broadly comparable; benchmarks can put either ahead.
3. **POSIX Message Queue** — kernel copy plus priority-ordered insertion.
4. **TCP/UDP socket over the network** — adds protocol processing and, for real
   remote peers, the network itself.

### Common Pitfalls
- **Shared memory without synchronization** — Race conditions are guaranteed. Always pair with semaphore or mutex.
- **Not closing unused pipe ends** — A reader that still holds the write end open never sees EOF (`read()` never returns 0), and a writer that still holds the read end open blocks forever on a full pipe instead of failing. Closing unnecessary duplicate descriptors after `fork()` is what makes EOF and `SIGPIPE`/`EPIPE` arrive when they should. (`SIGPIPE` is raised only once *all* read ends are closed — see `man 7 pipe`.)
- **Forgetting `mq_unlink()`/`shm_unlink()`** — IPC objects persist until unlinked or reboot, even after every process exits.
- **Using `printf()` in signal handlers** — Not async-signal-safe. Use `write()` instead (`man 7 signal-safety`).
- **Signal handler race conditions** — Use `volatile sig_atomic_t` for flags.
- **Assuming `mq_notify()` fires on every message** — It fires only when a message arrives at an *empty* queue, and the registration is removed after each delivery, so it must be re-registered.
- **Deadlock with semaphores** — Always acquire in consistent order; use timeouts.

### POSIX vs System V IPC
This repository uses POSIX IPC throughout, because the API is cleaner:
- File-descriptor-based on Linux, so queues and shared memory work with `select()`/`poll()`/`epoll()`
- Visible in the filesystem (`/dev/shm/`, `/dev/mqueue/`), which makes debugging far easier
- Simpler, more consistent naming (`/name`) instead of System V `key_t` values

Note on portability: POSIX IPC is *not* universally the more portable choice.
System V IPC is available on a wider range of older UNIX systems, and POSIX
message queues in particular are missing on some platforms (for example macOS).
System V IPC (`shmget`, `semget`, `msgget`) is still common in existing codebases.

### Key Limits (Linux defaults — verify on your system)

| Limit | Default | Where to check |
|---|---|---|
| Pipe capacity | 65536 bytes (16 pages) | `fcntl(fd, F_GETPIPE_SZ)`; ceiling in `/proc/sys/fs/pipe-max-size` |
| `PIPE_BUF` (atomic write size) | 4096 bytes | `<limits.h>` |
| Message queue max messages | 10 | `/proc/sys/fs/mqueue/msg_max` |
| Message queue max message size | 8192 bytes | `/proc/sys/fs/mqueue/msgsize_max` |
| Message priority range | 0–32767 | `sysconf(_SC_MQ_PRIO_MAX) - 1` |

`man 7 pipe` warns explicitly that applications should not rely on a particular
pipe capacity, so treat all of these as defaults, not guarantees.

### Further Reading
- *The Linux Programming Interface* (Kerrisk): Ch. 20–22 (signals), 43–44 (IPC overview, pipes/FIFOs), 51–54 (POSIX IPC), 56–61 (sockets)
- `man 7 pipe`, `man 7 fifo`, `man 7 mq_overview`, `man 7 shm_overview`, `man 7 sem_overview`, `man 7 unix`, `man 7 signal-safety`
- Linux kernel source: `fs/pipe.c`, `ipc/mqueue.c`, `mm/shmem.c`
