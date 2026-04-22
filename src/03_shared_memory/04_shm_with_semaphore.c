/*
 * 04_shm_with_semaphore.c — Synchronized Shared Memory Access
 *
 * The correct way to use shared memory: combine it with a semaphore
 * to prevent race conditions. This is the pattern used in production
 * systems like database buffer pools and multimedia pipelines.
 *
 * Pattern:
 *   sem_wait(&mutex)  →  read/write shared memory  →  sem_post(&mutex)
 *
 * Build: gcc -o 04_shm_with_semaphore 04_shm_with_semaphore.c -lrt -lpthread
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <semaphore.h>

#define SHM_NAME "/ipc_demo_safe"
#define SEM_NAME "/ipc_demo_safe_sem"

typedef struct {
    int  value;
    char message[128];
} shared_data_t;

int main(void)
{
    /* Create shared memory */
    int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    ftruncate(fd, sizeof(shared_data_t));
    shared_data_t *data = mmap(NULL, sizeof(shared_data_t),
                                PROT_READ | PROT_WRITE,
                                MAP_SHARED, fd, 0);
    close(fd);
    memset(data, 0, sizeof(shared_data_t));

    /* Create named semaphore (initial value = 1 → unlocked) */
    sem_unlink(SEM_NAME);
    sem_t *mutex = sem_open(SEM_NAME, O_CREAT, 0666, 1);
    if (mutex == SEM_FAILED) {
        perror("sem_open");
        return EXIT_FAILURE;
    }

    printf("=== Shared Memory + Semaphore Demo ===\n\n");

    pid_t pid = fork();

    if (pid == 0) {
        /* --- CHILD: Writer --- */
        for (int i = 1; i <= 5; i++) {
            sem_wait(mutex);  /* Lock */

            data->value = i * 10;
            snprintf(data->message, sizeof(data->message),
                     "Update #%d from writer (PID %d)", i, getpid());
            printf("[Writer] Wrote: value=%d, msg=\"%s\"\n",
                   data->value, data->message);

            sem_post(mutex);  /* Unlock */
            usleep(200000);
        }

        sem_close(mutex);
        munmap(data, sizeof(shared_data_t));
        return EXIT_SUCCESS;
    }

    /* --- PARENT: Reader --- */
    for (int i = 0; i < 5; i++) {
        usleep(250000);

        sem_wait(mutex);  /* Lock */

        printf("[Reader] Read:  value=%d, msg=\"%s\"\n",
               data->value, data->message);

        sem_post(mutex);  /* Unlock */
    }

    wait(NULL);

    /* Clean up all IPC resources */
    sem_close(mutex);
    sem_unlink(SEM_NAME);
    munmap(data, sizeof(shared_data_t));
    shm_unlink(SHM_NAME);
    printf("\n[Done] All IPC resources cleaned up.\n");

    return EXIT_SUCCESS;
}
