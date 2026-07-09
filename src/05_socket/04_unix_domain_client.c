/*
 * 04_unix_domain_client.c — UNIX Domain Socket Client
 *
 * Connects to the UNIX domain server via the filesystem path, sends a few
 * messages, and prints the reversed responses.
 *
 * Build: gcc -Wall -Wextra -o 04_unix_domain_client 04_unix_domain_client.c
 * Run:   ./04_unix_domain_client   (after starting 03_unix_domain_server)
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
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("socket");
        return EXIT_FAILURE;
    }

    struct sockaddr_un addr = { .sun_family = AF_UNIX };
    strncpy(addr.sun_path, SOCK_PATH, sizeof(addr.sun_path) - 1);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("connect (is the server running?)");
        close(sock);
        return EXIT_FAILURE;
    }

    printf("[UDS Client] Connected to %s\n", SOCK_PATH);

    const char *messages[] = { "Hello", "Linux IPC", "Unix Socket", NULL };
    char buf[BUFSIZE];

    for (int i = 0; messages[i] != NULL; i++) {
        if (send(sock, messages[i], strlen(messages[i]), 0) == -1) {
            perror("send");
            break;
        }
        printf("[UDS Client] Sent:              \"%s\"\n", messages[i]);

        ssize_t n = recv(sock, buf, sizeof(buf) - 1, 0);
        if (n <= 0)
            break;

        buf[n] = '\0';
        printf("[UDS Client] Reply (reversed):  \"%s\"\n", buf);

        usleep(300000);
    }

    close(sock);
    printf("[UDS Client] Done.\n");
    return EXIT_SUCCESS;
}
