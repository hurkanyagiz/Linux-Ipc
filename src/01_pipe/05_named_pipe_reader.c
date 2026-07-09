/*
 * 05_named_pipe_reader.c — FIFO Reader
 *
 * Opens an existing named pipe and reads messages from it.
 * Run 04_named_pipe_writer first in another terminal.
 *
 * Build: gcc -o 05_named_pipe_reader 05_named_pipe_reader.c
 * Usage: ./05_named_pipe_reader
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

#define FIFO_PATH "/tmp/ipc_demo_fifo"

int main(void)
{
    /* Create FIFO if it doesn't exist (in case reader starts first) */
    if (mkfifo(FIFO_PATH, 0666) == -1 && errno != EEXIST) {
        perror("mkfifo");
        return EXIT_FAILURE;
    }

    printf("[Reader] Opening FIFO: %s\n", FIFO_PATH);
    printf("[Reader] Waiting for writer to connect...\n");

    int fd = open(FIFO_PATH, O_RDONLY);
    if (fd == -1) {
        perror("open");
        return EXIT_FAILURE;
    }

    printf("[Reader] Writer connected! Waiting for messages...\n\n");

    char buffer[256];
    ssize_t n;
    int count = 0;
    while ((n = read(fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[n] = '\0';
        printf("[Reader] Message #%d: \"%s\"\n", ++count, buffer);
    }

    close(fd);
    printf("\n[Reader] Writer closed the pipe. Total messages: %d\n", count);

    return EXIT_SUCCESS;
}
