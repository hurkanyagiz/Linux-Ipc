/*
 * 05_udp_echo.c — Connectionless UDP Echo (server + client in one binary)
 *
 * UDP is connectionless: there is no handshake and no connection state.
 * Each sendto() delivers one independent datagram. This makes UDP faster
 * than TCP but unreliable — packets can be lost, duplicated, or reordered.
 *
 * Usage:
 *   ./05_udp_echo server    (terminal 1)
 *   ./05_udp_echo client    (terminal 2)
 *
 * Build: gcc -Wall -Wextra -o 05_udp_echo 05_udp_echo.c
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT    9090
#define BUFSIZE 512

static void run_server(void)
{
    /* Line-buffer stdout so log lines appear even when redirected */
    setvbuf(stdout, NULL, _IOLBF, 0);

    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in addr = {
        .sin_family      = AF_INET,
        .sin_addr.s_addr = INADDR_ANY,
        .sin_port        = htons(PORT)
    };

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    printf("[UDP Server] Listening on port %d (Ctrl+C to stop)\n\n", PORT);

    char buf[BUFSIZE];
    struct sockaddr_in client;
    socklen_t client_len = sizeof(client);

    while (1) {
        ssize_t n = recvfrom(fd, buf, sizeof(buf) - 1, 0,
                             (struct sockaddr *)&client, &client_len);
        if (n <= 0)
            continue;

        buf[n] = '\0';

        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client.sin_addr, ip, sizeof(ip));
        printf("[UDP Server] From %s: \"%s\"\n", ip, buf);

        /* Echo the datagram straight back to the sender */
        sendto(fd, buf, (size_t)n, 0,
               (struct sockaddr *)&client, client_len);
    }
}

static void run_client(void)
{
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server = {
        .sin_family = AF_INET,
        .sin_port   = htons(PORT)
    };
    inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);

    const char *messages[] = {
        "Hello UDP",
        "No handshake needed",
        "Fast but unreliable",
        NULL
    };

    char buf[BUFSIZE];

    for (int i = 0; messages[i] != NULL; i++) {
        sendto(fd, messages[i], strlen(messages[i]), 0,
               (struct sockaddr *)&server, sizeof(server));
        printf("[UDP Client] Sent: \"%s\"\n", messages[i]);

        ssize_t n = recvfrom(fd, buf, sizeof(buf) - 1, 0, NULL, NULL);
        if (n > 0) {
            buf[n] = '\0';
            printf("[UDP Client] Echo: \"%s\"\n", buf);
        }

        usleep(500000);
    }

    close(fd);
    printf("[UDP Client] Done.\n");
}

int main(int argc, char *argv[])
{
    if (argc != 2 ||
        (strcmp(argv[1], "server") != 0 && strcmp(argv[1], "client") != 0)) {
        fprintf(stderr, "Usage: %s server|client\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (strcmp(argv[1], "server") == 0)
        run_server();
    else
        run_client();

    return EXIT_SUCCESS;
}
