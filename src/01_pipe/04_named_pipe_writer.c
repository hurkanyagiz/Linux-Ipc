/*
 * 04_named_pipe_writer.c — FIFO Writer
 *
 * Creates a named pipe (FIFO) and writes messages to it.
 * Run this in one terminal, and 05_named_pipe_reader in another.
 *
 * Unlike unnamed pipes, FIFOs have a name in the filesystem and
 * can be used between completely unrelated processes.
 *
 * Build: gcc -o 04_named_pipe_writer 04_named_pipe_writer.c
 * Usage: ./04_named_pipe_writer
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

#define FIFO_PATH "/tmp/ipc_demo_fifo"

int main(void)
{
    /* Create the FIFO if it doesn't exist */
    if (mkfifo(FIFO_PATH, 0666) == -1 && errno != EEXIST) {
        perror("mkfifo");
        return EXIT_FAILURE;
    }

    printf("[Writer] FIFO created at: %s\n", FIFO_PATH);
    printf("[Writer] Waiting for reader to connect...\n");

    /* open() blocks until a reader opens the other end */
    int fd = open(FIFO_PATH, O_WRONLY);
    if (fd == -1) {
        perror("open");
        return EXIT_FAILURE;
    }

    printf("[Writer] Reader connected! Type messages (or 'quit' to exit):\n");

    char input[256];
    while (1) {
        printf("> ");
        if (fgets(input, sizeof(input), stdin) == NULL)
            break;
        input[strcspn(input, "\n")] = '\0';

        if (strcmp(input, "quit") == 0)
            break;

        if (write(fd, input, strlen(input)) == -1) {
            perror("write");
            break;
        }
    }

    close(fd);
    unlink(FIFO_PATH); /* Clean up the FIFO file */
    printf("[Writer] FIFO closed and removed.\n");

    return EXIT_SUCCESS;
}
