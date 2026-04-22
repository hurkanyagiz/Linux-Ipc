/*
 * 01_basic_handler.c — Register and Handle Common Signals
 *
 * Shows how to use sigaction() (preferred over signal()) to register
 * handlers for SIGINT, SIGTERM, SIGUSR1, and SIGUSR2.
 *
 * Test: Run this program, then from another terminal:
 *   kill -SIGUSR1 <pid>
 *   kill -SIGUSR2 <pid>
 *   kill -SIGTERM <pid>
 *   Or press Ctrl+C for SIGINT
 *
 * Build: gcc -o 01_basic_handler 01_basic_handler.c
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

static volatile sig_atomic_t got_usr1 = 0;
static volatile sig_atomic_t got_usr2 = 0;
static volatile sig_atomic_t shutdown_req = 0;

static void handler(int signo) {
    /* Only use async-signal-safe functions here (write, not printf) */
    switch (signo) {
    case SIGINT:
        if (shutdown_req) _exit(0);
        shutdown_req = 1;
        break;
    case SIGTERM: shutdown_req = 1; break;
    case SIGUSR1: got_usr1 = 1; break;
    case SIGUSR2: got_usr2 = 1; break;
    }
}

int main(void) {
    printf("=== Signal Handler Demo ===\n");
    printf("PID: %d\n\n", getpid());
    printf("Test commands (from another terminal):\n");
    printf("  kill -SIGUSR1 %d\n", getpid());
    printf("  kill -SIGUSR2 %d\n", getpid());
    printf("  kill -SIGTERM %d\n", getpid());
    printf("  Or press Ctrl+C\n\n");

    /* Register handlers using sigaction (more reliable than signal()) */
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT,  &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGUSR2, &sa, NULL);

    int tick = 0;
    while (!shutdown_req) {
        if (got_usr1) { printf("[Signal] SIGUSR1 received!\n"); got_usr1 = 0; }
        if (got_usr2) { printf("[Signal] SIGUSR2 received!\n"); got_usr2 = 0; }
        printf("\r[Running] tick=%d  ", tick++);
        fflush(stdout);
        sleep(1);
    }
    printf("\n\n[Main] Received shutdown signal. Cleaning up...\n");
    printf("[Main] Goodbye.\n");
    return 0;
}
