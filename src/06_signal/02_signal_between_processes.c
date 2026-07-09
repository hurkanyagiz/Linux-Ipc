/*
 * 02_signal_between_processes.c — Parent-Child Signal Coordination
 *
 * Parent and child use SIGUSR1/SIGUSR2 as a simple signaling protocol:
 *   1. Parent sends SIGUSR1 to child → "start working"
 *   2. Child does work, sends SIGUSR2 to parent → "done"
 *   3. Parent sends SIGUSR1 → "terminate"
 *
 * Build: gcc -o 02_signal_between_processes 02_signal_between_processes.c
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

static volatile sig_atomic_t sig_received = 0;
static void on_signal(int s) { (void)s; sig_received = 1; }

int main(void) {
    struct sigaction sa = {.sa_handler = on_signal};
    sigemptyset(&sa.sa_mask);
    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGUSR2, &sa, NULL);

    printf("=== Inter-Process Signal Coordination ===\n\n");

    pid_t pid = fork();
    if (pid == 0) {
        /* CHILD */
        printf("[Child  PID=%d] Waiting for parent's signal...\n", getpid());
        while (!sig_received) pause();
        sig_received = 0;

        printf("[Child  PID=%d] Got SIGUSR1! Working...\n", getpid());
        sleep(1);

        printf("[Child  PID=%d] Done. Signaling parent (SIGUSR2).\n", getpid());
        kill(getppid(), SIGUSR2);

        while (!sig_received) pause();
        printf("[Child  PID=%d] Got termination signal. Exiting.\n", getpid());
        return 0;
    }

    /* PARENT */
    printf("[Parent PID=%d] Child created: PID=%d\n\n", getpid(), pid);
    sleep(1);

    printf("[Parent] Sending SIGUSR1 to child (start)...\n");
    kill(pid, SIGUSR1);

    printf("[Parent] Waiting for child's response...\n");
    while (!sig_received) pause();
    sig_received = 0;

    printf("[Parent] Got SIGUSR2 — child finished work.\n");
    printf("[Parent] Sending final SIGUSR1 (terminate)...\n");
    kill(pid, SIGUSR1);

    int status;
    waitpid(pid, &status, 0);
    printf("\n[Parent] Child exited (code=%d). All done.\n", WEXITSTATUS(status));
    return 0;
}
