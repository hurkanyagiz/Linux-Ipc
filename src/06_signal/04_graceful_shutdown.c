/*
 * 04_graceful_shutdown.c — Clean Resource Cleanup on SIGTERM/SIGINT
 *
 * Demonstrates the pattern used in production daemons:
 *   1. Catch SIGTERM/SIGINT
 *   2. Set a flag (volatile sig_atomic_t)
 *   3. Main loop checks the flag and exits cleanly
 *   4. Cleanup handler releases all resources
 *
 * Build: gcc -o 04_graceful_shutdown 04_graceful_shutdown.c -lrt
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/mman.h>

#define SHM_NAME "/ipc_graceful_demo"
#define TMPFILE  "/tmp/ipc_graceful_demo.tmp"

static volatile sig_atomic_t shutdown_flag = 0;
static int tmpfd = -1;

static void shutdown_handler(int sig) {
    (void)sig;
    shutdown_flag = 1;
}

static void cleanup(void) {
    printf("[Cleanup] Releasing resources...\n");
    if (tmpfd >= 0) { close(tmpfd); unlink(TMPFILE); printf("  - Temp file removed\n"); }
    shm_unlink(SHM_NAME); printf("  - Shared memory unlinked\n");
    printf("[Cleanup] Done. Clean exit.\n");
}

int main(void) {
    /* Register signal handlers */
    struct sigaction sa = {.sa_handler = shutdown_handler};
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    /* Register cleanup at exit */
    atexit(cleanup);

    /* Allocate some resources (to demonstrate cleanup) */
    tmpfd = open(TMPFILE, O_CREAT | O_RDWR, 0644);
    int shmfd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (ftruncate(shmfd, 1024) == -1)
        perror("ftruncate");
    close(shmfd);

    printf("=== Graceful Shutdown Demo ===\n");
    printf("PID: %d\n", getpid());
    printf("Resources allocated: temp file, shared memory\n");
    printf("Press Ctrl+C or: kill -SIGTERM %d\n\n", getpid());

    int iteration = 0;
    while (!shutdown_flag) {
        printf("[Working] iteration %d...\n", ++iteration);
        sleep(1);
    }

    printf("\n[Main] Shutdown signal received. Exiting cleanly.\n");
    /* atexit(cleanup) runs automatically */
    return 0;
}
