/*
 * 02_bidirectional_pipe.c — Two-Way Communication Using Two Pipes
 *
 * Since pipes are unidirectional, bidirectional communication requires
 * two separate pipes. This example shows parent and child exchanging
 * messages in both directions.
 *
 * Pipe layout:
 *   pipe1: Parent writes → Child reads
 *   pipe2: Child writes  → Parent reads
 *
 * Build: gcc -o 02_bidirectional_pipe 02_bidirectional_pipe.c
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define BUF_SIZE 256

int main(void)
{
    int pipe_parent_to_child[2]; /* Parent → Child */
    int pipe_child_to_parent[2]; /* Child → Parent */

    if (pipe(pipe_parent_to_child) == -1 || pipe(pipe_child_to_parent) == -1) {
        perror("pipe");
        return EXIT_FAILURE;
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return EXIT_FAILURE;
    }

    if (pid == 0) {
        /* --- CHILD --- */
        close(pipe_parent_to_child[1]); /* Close write end of pipe1 */
        close(pipe_child_to_parent[0]); /* Close read end of pipe2  */

        char buf[BUF_SIZE];
        ssize_t n;

        /* Read question from parent */
        n = read(pipe_parent_to_child[0], buf, sizeof(buf) - 1);
        buf[n] = '\0';
        printf("[Child ] Received question: \"%s\"\n", buf);

        /* Send answer back to parent */
        const char *answer = "I am process 42, running since boot.";
        printf("[Child ] Sending answer: \"%s\"\n", answer);
        write(pipe_child_to_parent[1], answer, strlen(answer));

        close(pipe_parent_to_child[0]);
        close(pipe_child_to_parent[1]);
        return EXIT_SUCCESS;
    }

    /* --- PARENT --- */
    close(pipe_parent_to_child[0]); /* Close read end of pipe1  */
    close(pipe_child_to_parent[1]); /* Close write end of pipe2 */

    /* Send question to child */
    const char *question = "What is your status?";
    printf("[Parent] Sending question: \"%s\"\n", question);
    write(pipe_parent_to_child[1], question, strlen(question));

    /* Read answer from child */
    char buf[BUF_SIZE];
    ssize_t n = read(pipe_child_to_parent[0], buf, sizeof(buf) - 1);
    buf[n] = '\0';
    printf("[Parent] Received answer: \"%s\"\n", buf);

    close(pipe_parent_to_child[1]);
    close(pipe_child_to_parent[0]);
    wait(NULL);

    return EXIT_SUCCESS;
}
