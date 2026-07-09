/*
 * 01_sem_basic.c — Binary Semaphore Between Two Processes
 *
 * Demonstrates the fundamental semaphore operations: sem_wait() to lock
 * and sem_post() to unlock. Parent and child take turns accessing a
 * "critical section" protected by a named semaphore.
 *
 * Build: gcc -o 01_sem_basic 01_sem_basic.c -lpthread -lrt
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <fcntl.h>

#define SEM_NAME "/ipc_demo_sem_basic"

/* Simulate a critical section that must not be entered concurrently */
static void critical_section(const char *who, int iteration)
{
    printf("[%s] >>> Entering critical section (iteration %d)\n", who, iteration);
    usleep(300000); /* Simulate work */
    printf("[%s] <<< Leaving critical section\n", who);
}

int main(void)
{
    sem_unlink(SEM_NAME);
    sem_t *sem = sem_open(SEM_NAME, O_CREAT, 0666, 1); /* Initial value 1 */
    if (sem == SEM_FAILED) {
        perror("sem_open");
        return EXIT_FAILURE;
    }

    printf("=== Binary Semaphore Demo ===\n\n");

    pid_t pid = fork();

    if (pid == 0) {
        /* --- CHILD --- */
        for (int i = 1; i <= 3; i++) {
            sem_wait(sem);            /* P() — decrement, block if 0 */
            critical_section("Child ", i);
            sem_post(sem);            /* V() — increment, unblock waiters */
            usleep(100000);
        }
        sem_close(sem);
        return EXIT_SUCCESS;
    }

    /* --- PARENT --- */
    for (int i = 1; i <= 3; i++) {
        sem_wait(sem);
        critical_section("Parent", i);
        sem_post(sem);
        usleep(100000);
    }

    wait(NULL);
    sem_close(sem);
    sem_unlink(SEM_NAME);
    printf("\n[Done] No concurrent access occurred — semaphore worked!\n");

    return EXIT_SUCCESS;
}
