/*
 * 02_shm_read.c — Read from Existing Shared Memory
 *
 * Opens a shared memory object created by 01_shm_write and reads its contents.
 * After reading, it cleans up the shared memory object.
 *
 * Build: gcc -o 02_shm_read 02_shm_read.c -lrt
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#define SHM_NAME "/ipc_demo_basic"
#define SHM_SIZE 4096

int main(void)
{
    /* Open existing shared memory (read-only) */
    int fd = shm_open(SHM_NAME, O_RDONLY, 0);
    if (fd == -1) {
        perror("shm_open (run 01_shm_write first)");
        return EXIT_FAILURE;
    }

    /* Map into our address space */
    char *ptr = mmap(NULL, SHM_SIZE, PROT_READ, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
        perror("mmap");
        return EXIT_FAILURE;
    }
    close(fd);

    /* Read data — it's just a pointer dereference, no system call! */
    printf("[Reader] PID %d reading shared memory: %s\n", getpid(), SHM_NAME);
    printf("[Reader] Content: \"%s\"\n", ptr);

    /* Clean up */
    munmap(ptr, SHM_SIZE);
    shm_unlink(SHM_NAME); /* Remove the shared memory object */
    printf("[Reader] Shared memory unlinked.\n");

    return EXIT_SUCCESS;
}
