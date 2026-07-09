/*
 * 02_tcp_client.c — Interactive TCP Client
 *
 * Connects to the local TCP server (01_tcp_server) and offers an
 * interactive prompt: every line typed is sent to the server and the
 * echoed response is printed.
 *
 * Key concepts:
 *   - socket() + connect() client lifecycle
 *   - Converting a printable IP address with inet_pton()
 *   - Detecting a server-side disconnect (recv() returns 0)
 *
 * Build: gcc -Wall -Wextra -o 02_tcp_client 02_tcp_client.c
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"
#define PORT      8080
#define BUFSIZE   1024

int main(void)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("socket");
        return EXIT_FAILURE;
    }

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port   = htons(PORT)
    };
    inet_pton(AF_INET, SERVER_IP, &addr.sin_addr);

    printf("[Client] Connecting to %s:%d...\n", SERVER_IP, PORT);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("connect (is the server running?)");
        close(sock);
        return EXIT_FAILURE;
    }

    /* Read the server's welcome message */
    char buf[BUFSIZE];
    ssize_t n = recv(sock, buf, sizeof(buf) - 1, 0);
    if (n > 0) {
        buf[n] = '\0';
        printf("[Server] %s", buf);
    }

    printf("[Client] Type messages ('quit' to exit):\n");

    while (1) {
        printf("> ");
        if (fgets(buf, sizeof(buf), stdin) == NULL)
            break;

        buf[strcspn(buf, "\n")] = '\0';
        if (strlen(buf) == 0)
            continue;

        if (send(sock, buf, strlen(buf), 0) == -1) {
            perror("send");
            break;
        }

        if (strcmp(buf, "quit") == 0)
            break;

        n = recv(sock, buf, sizeof(buf) - 1, 0);
        if (n <= 0) {
            printf("[Client] Server closed the connection.\n");
            break;
        }
        buf[n] = '\0';
        printf("[Server] %s", buf);
    }

    close(sock);
    printf("[Client] Disconnected.\n");
    return EXIT_SUCCESS;
}
