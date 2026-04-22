/*
 * 03_pipe_redirect.c — Redirect Child's stdout Through a Pipe
 *
 * Shows how shell pipelines work internally: redirect a child process's
 * stdout to a pipe so the parent can capture its output programmatically.
 *
 * This is the mechanism behind: ls -la | grep ".c"
 *
 * Key concept: dup2(pipefd[1], STDOUT_FILENO) makes the pipe's write end
 * become the child's stdout. Any printf/write to stdout goes into the pipe.
 *
 * Build: gcc -o 03_pipe_redirect 03_pipe_redirect.c
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void)
{
    int pipefd[2];

    if (pipe(pipefd) == -1) {
        perror("pipe");
        return EXIT_FAILURE;
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return EXIT_FAILURE;
    }

    if (pid == 0) {
        /* --- CHILD: Execute "ls -la" with stdout redirected to pipe --- */
        close(pipefd[0]); /* Close read end */

        /* Replace stdout (fd 1) with the pipe's write end */
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]); /* Original fd no longer needed */

        /* exec replaces this process image with "ls" */
        execlp("ls", "ls", "-la", "--color=never", (char *)NULL);
        perror("execlp"); /* Only reached if exec fails */
        return EXIT_FAILURE;
    }

    /* --- PARENT: Read child's stdout output from the pipe --- */
    close(pipefd[1]); /* Close write end */

    printf("[Parent] Captured output from child (ls -la):\n");
    printf("─────────────────────────────────────────────\n");

    char buf[4096];
    ssize_t n;
    while ((n = read(pipefd[0], buf, sizeof(buf) - 1)) > 0) {
        buf[n] = '\0';
        printf("%s", buf);
    }

    printf("─────────────────────────────────────────────\n");

    close(pipefd[0]);
    wait(NULL);

    return EXIT_SUCCESS;
}
