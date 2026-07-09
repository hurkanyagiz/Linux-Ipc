/*
 * 01_shm_write.c — Create Shared Memory and Write Data
 *
 * Creates a POSIX shared memory object, maps it into this process's
 * address space, and writes a message. Run 02_shm_read to read it.
 *
 * The shared memory object is visible at /dev/shm/ipc_demo_basic
 *
 * Build: gcc -o 01_shm_write 01_shm_write.c -lrt
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#define SHM_NAME "/ipc_demo_basic"
#define SHM_SIZE 4096

int main(void)
{
    /* Step 1: Create the shared memory object */
    int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        perror("shm_open");
        return EXIT_FAILURE;
    }

    /* Step 2: Set its size */
    if (ftruncate(fd, SHM_SIZE) == -1) {
        perror("ftruncate");
        return EXIT_FAILURE;
    }

    /* Step 3: Map it into our address space */
    char *ptr = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE,
                     MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
        perror("mmap");
        return EXIT_FAILURE;
    }
    close(fd); /* fd no longer needed after mmap */

    /* Step 4: Write data directly to shared memory */
    const char *message = "Hello from shared memory! PID: ";
    sprintf(ptr, "%s%d", message, getpid());

    printf("[Writer] Created shared memory: %s\n", SHM_NAME);
    printf("[Writer] Wrote: \"%s\"\n", ptr);
    printf("[Writer] You can see it: cat /dev/shm%s\n", SHM_NAME);
    printf("[Writer] Run 02_shm_read to read it from another process.\n");

    /* Unmap (but don't unlink — reader needs it) */
    munmap(ptr, SHM_SIZE);

    return EXIT_SUCCESS;
}
