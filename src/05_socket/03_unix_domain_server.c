/*
 * 03_unix_domain_server.c — UNIX Domain Socket Server
 *
 * UNIX domain sockets use a filesystem path instead of an IP:port pair.
 * They bypass the network stack entirely, which makes them faster than
 * TCP for same-host communication. Docker, PostgreSQL, and Nginx all use
 * them for local connections.
 *
 * This server accepts one client and echoes every message back reversed.
 *
 * Build: gcc -Wall -Wextra -o 03_unix_domain_server 03_unix_domain_server.c
 * Run:   ./03_unix_domain_server   (then start 04_unix_domain_client)
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCK_PATH "/tmp/ipc_demo_uds.sock"
#define BUFSIZE   256

int main(void)
{
    /* Remove any stale socket file from a previous run */
    unlink(SOCK_PATH);

    int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        return EXIT_FAILURE;
    }

    struct sockaddr_un addr = { .sun_family = AF_UNIX };
    strncpy(addr.sun_path, SOCK_PATH, sizeof(addr.sun_path) - 1);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("bind");
        close(server_fd);
        return EXIT_FAILURE;
    }

    if (listen(server_fd, 1) == -1) {
        perror("listen");
        close(server_fd);
        return EXIT_FAILURE;
    }

    printf("[UDS Server] Listening on: %s\n", SOCK_PATH);
    printf("[UDS Server] Run 04_unix_domain_client in another terminal.\n\n");

    int client_fd = accept(server_fd, NULL, NULL);
    if (client_fd == -1) {
        perror("accept");
        close(server_fd);
        return EXIT_FAILURE;
    }

    printf("[UDS Server] Client connected.\n");

    char buf[BUFSIZE];
    ssize_t n;
    while ((n = recv(client_fd, buf, sizeof(buf) - 1, 0)) > 0) {
        buf[n] = '\0';
        printf("[UDS Server] Received: \"%s\"\n", buf);

        /* Reverse the string and send it back */
        char reversed[BUFSIZE];
        int len = (int)strlen(buf);
        for (int i = 0; i < len; i++)
            reversed[i] = buf[len - 1 - i];
        reversed[len] = '\0';

        if (send(client_fd, reversed, (size_t)len, 0) == -1) {
            perror("send");
            break;
        }
    }

    close(client_fd);
    close(server_fd);
    unlink(SOCK_PATH);
    printf("[UDS Server] Done, socket removed.\n");
    return EXIT_SUCCESS;
}
