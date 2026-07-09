/*
 * 03_shm_struct.c — Share a C Struct Between Processes
 *
 * Maps a struct into shared memory so parent and child can both
 * read/write the same structured data. This is the real power of
 * shared memory — you can share complex data structures directly.
 *
 * WARNING: No synchronization here! See 04_shm_with_semaphore.c
 * for the safe version with semaphore protection.
 *
 * Build: gcc -o 03_shm_struct 03_shm_struct.c -lrt
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <time.h>

#define SHM_NAME "/ipc_demo_struct"

/* Shared data structure — both processes see the same memory layout */
typedef struct {
    int    counter;
    double temperature;
    char   status[64];
    pid_t  last_writer;
    time_t last_update;
} sensor_data_t;

int main(void)
{
    /* Create and size the shared memory */
    int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (ftruncate(fd, sizeof(sensor_data_t)) == -1) {
        perror("ftruncate");
        return EXIT_FAILURE;
    }

    sensor_data_t *data = mmap(NULL, sizeof(sensor_data_t),
                                PROT_READ | PROT_WRITE,
                                MAP_SHARED, fd, 0);
    close(fd);

    /* Initialize */
    memset(data, 0, sizeof(sensor_data_t));

    pid_t pid = fork();

    if (pid == 0) {
        /* --- CHILD: Write sensor readings --- */
        for (int i = 0; i < 5; i++) {
            data->counter = i + 1;
            data->temperature = 20.0 + (i * 1.5);
            snprintf(data->status, sizeof(data->status),
                     "Reading #%d: %.1f°C", i + 1, data->temperature);
            data->last_writer = getpid();
            data->last_update = time(NULL);

            printf("[Child ] Wrote: counter=%d, temp=%.1f°C\n",
                   data->counter, data->temperature);
            usleep(300000);
        }
        munmap(data, sizeof(sensor_data_t));
        return EXIT_SUCCESS;
    }

    /* --- PARENT: Read sensor data --- */
    usleep(100000); /* Let child write first */

    for (int i = 0; i < 5; i++) {
        usleep(300000);
        printf("[Parent] Read: counter=%d, temp=%.1f°C, status=\"%s\"\n",
               data->counter, data->temperature, data->status);
    }

    wait(NULL);
    munmap(data, sizeof(sensor_data_t));
    shm_unlink(SHM_NAME);

    return EXIT_SUCCESS;
}
