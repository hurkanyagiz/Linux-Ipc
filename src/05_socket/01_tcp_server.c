/*
 * 01_tcp_server.c — Multi-Client TCP Server
 *
 * Accepts multiple clients concurrently using the classic fork-per-client
 * model. Each connection is handled by its own child process, while the
 * parent keeps listening for new connections.
 *
 * Key concepts:
 *   - socket() / bind() / listen() / accept() lifecycle
 *   - SO_REUSEADDR so the port can be reused immediately after restart
 *   - SIGCHLD handler to reap finished children (avoid zombies)
 *   - fork() to isolate each client session
 *
 * Build: gcc -Wall -Wextra -o 01_tcp_server 01_tcp_server.c
 * Test:  telnet localhost 8080   (or run 02_tcp_client)
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <ctype.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT    8080
#define BACKLOG 5
#define BUFSIZE 1024

static volatile sig_atomic_t running = 1;

static void handle_shutdown(int sig)
{
    (void)sig;
    running = 0;
}

/* Reap terminated children so they don't become zombies */
static void reap_children(int sig)
{
    (void)sig;
    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;
}

static void handle_client(int client_fd, const char *client_ip)
{
    char buf[BUFSIZE];
    const char *welcome = "Welcome! Type messages ('quit' to exit).\n";

    if (send(client_fd, welcome, strlen(welcome), 0) == -1) {
        perror("send");
        close(client_fd);
        return;
    }

    ssize_t n;
    while ((n = recv(client_fd, buf, sizeof(buf) - 1, 0)) > 0) {
        buf[n] = '\0';
        buf[strcspn(buf, "\r\n")] = '\0';

        if (strcmp(buf, "quit") == 0)
            break;

        printf("[Server] %s: \"%s\"\n", client_ip, buf);

        /* Build an uppercase echo response */
        char resp[BUFSIZE + 8];
        int len = snprintf(resp, sizeof(resp), "ECHO: ");
        for (int i = 0; buf[i] != '\0' && len < (int)sizeof(resp) - 2; i++)
            resp[len++] = (char)toupper((unsigned char)buf[i]);
        resp[len++] = '\n';
        resp[len] = '\0';

        if (send(client_fd, resp, (size_t)len, 0) == -1) {
            perror("send");
            break;
        }
    }

    close(client_fd);
}

int main(void)
{
    /* Install signal handlers */
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_shutdown;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    sa.sa_handler = reap_children;
    sa.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &sa, NULL);

    /* Writing to a closed socket raises SIGPIPE by default; ignore it */
    signal(SIGPIPE, SIG_IGN);

    /* Create the listening socket */
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        return EXIT_FAILURE;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {
        .sin_family      = AF_INET,
        .sin_addr.s_addr = INADDR_ANY,
        .sin_port        = htons(PORT)
    };

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("bind");
        close(server_fd);
        return EXIT_FAILURE;
    }

    if (listen(server_fd, BACKLOG) == -1) {
        perror("listen");
        close(server_fd);
        return EXIT_FAILURE;
    }

    printf("=== TCP Server listening on port %d ===\n", PORT);
    printf("Connect with: telnet localhost %d  (or ./02_tcp_client)\n", PORT);
    printf("Press Ctrl+C to stop.\n\n");

    while (running) {
        struct sockaddr_in client;
        socklen_t client_len = sizeof(client);

        int client_fd = accept(server_fd,
                               (struct sockaddr *)&client, &client_len);
        if (client_fd < 0)
            continue; /* interrupted by signal or transient error */

        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client.sin_addr, ip, sizeof(ip));
        printf("[Server] Client connected: %s\n", ip);

        pid_t pid = fork();
        if (pid == 0) {
            /* Child: handle this client only */
            close(server_fd);
            handle_client(client_fd, ip);
            fflush(stdout); /* _exit() skips stdio flushing */
            _exit(EXIT_SUCCESS);
        } else if (pid > 0) {
            /* Parent: keep listening */
            close(client_fd);
        } else {
            perror("fork");
            close(client_fd);
        }
    }

    close(server_fd);
    printf("\n[Server] Shut down cleanly.\n");
    return EXIT_SUCCESS;
}
