/*
 * 01_basic_pipe.c — Minimal Pipe Example
 *
 * Demonstrates the simplest use of pipe(): parent writes a message,
 * child reads and prints it.
 *
 * Key concepts:
 *   - pipe() creates two file descriptors: fd[0] for reading, fd[1] for writing
 *   - fork() duplicates file descriptors — both parent and child have the pipe
 *   - Close unused ends to avoid resource leaks and ensure proper EOF
 *
 * Build: gcc -o 01_basic_pipe 01_basic_pipe.c
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void)
{
    int   pipefd[2]; /* pipefd[0] = read end, pipefd[1] = write end */
    pid_t pid;
    char  buffer[128];

    /* Step 1: Create the pipe BEFORE forking */
    if (pipe(pipefd) == -1) {
        perror("pipe");
        return EXIT_FAILURE;
    }

    /* Step 2: Fork — child inherits both file descriptors */
    pid = fork();
    if (pid == -1) {
        perror("fork");
        return EXIT_FAILURE;
    }

    if (pid == 0) {
        /* --- CHILD PROCESS (Reader) --- */
        close(pipefd[1]); /* Close write end — we only read */

        ssize_t n = read(pipefd[0], buffer, sizeof(buffer) - 1);
        if (n > 0) {
            buffer[n] = '\0';
            printf("[Child ] Received: \"%s\"\n", buffer);
        }

        close(pipefd[0]);
        return EXIT_SUCCESS;
    }

    /* --- PARENT PROCESS (Writer) --- */
    close(pipefd[0]); /* Close read end — we only write */

    const char *msg = "Hello from parent process!";
    printf("[Parent] Sending: \"%s\"\n", msg);
    write(pipefd[1], msg, strlen(msg));

    close(pipefd[1]); /* Close write end → child gets EOF */
    wait(NULL);       /* Wait for child to finish */
    printf("[Parent] Done.\n");

    return EXIT_SUCCESS;
}
