/*
 * task_client.c — CLI Interface (Capstone Project)
 *
 * IPC methods used:
 *   Socket — Send commands to dispatcher and receive responses
 *
 * Usage: ./task_client
 * Build: gcc -o task_client task_client.c
 */
#define _GNU_SOURCE
#include "../include/task_manager.h"

static int send_cmd(client_cmd_t *cmd, server_response_t *resp) {
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    struct sockaddr_un addr = {.sun_family = AF_UNIX};
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path)-1);
    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        fprintf(stderr, "Connection failed: is dispatcher running?\n");
        close(sock); return -1;
    }
    send(sock, cmd, sizeof(*cmd), 0);
    recv(sock, resp, sizeof(*resp), 0);
    close(sock);
    return 0;
}

static void print_help(void) {
    printf("\n"
    "  Commands:\n"
    "    submit <type> <priority> <param> [description]\n"
    "      type: compute | io | sleep\n"
    "      priority: 0-9 (9 = highest)\n"
    "\n"
    "    list       — Show all tasks\n"
    "    workers    — Show worker status\n"
    "    stats      — Show statistics\n"
    "    demo       — Submit 5 demo tasks\n"
    "    shutdown   — Shut down the system\n"
    "    help       — Show this menu\n"
    "    quit       — Exit client\n\n");
}

static void submit_demos(void) {
    struct { int type, prio, param; const char *desc; } demos[] = {
        {TASK_COMPUTE, 5, 10, "Fibonacci(10)"},
        {TASK_IO,      3,  3, "3-block IO job"},
        {TASK_SLEEP,   1,  2, "2-second wait"},
        {TASK_COMPUTE, 9, 15, "Fibonacci(15) URGENT"},
        {TASK_IO,      7,  5, "5-block critical IO"},
    };
    printf("[Demo] Submitting %d tasks...\n\n", 5);
    for (int i = 0; i < 5; i++) {
        client_cmd_t cmd = {.type=CMD_SUBMIT, .task_type=demos[i].type,
                            .priority=demos[i].prio, .param=demos[i].param};
        strncpy(cmd.description, demos[i].desc, TASK_DESC_LEN-1);
        server_response_t resp;
        if (send_cmd(&cmd, &resp) == 0)
            printf("  %s %s\n", resp.success ? "[OK]" : "[FAIL]", resp.message);
        usleep(100000);
    }
    printf("\n");
}

int main(void) {
    printf("=== Distributed Task Manager — Client ===\n");
    print_help();

    char line[512];
    while (1) {
        printf("task> "); fflush(stdout);
        if (!fgets(line, sizeof(line), stdin)) break;
        line[strcspn(line, "\n")] = '\0';
        if (!strlen(line)) continue;

        client_cmd_t cmd; server_response_t resp;
        memset(&cmd, 0, sizeof(cmd));

        if (strcmp(line,"quit")==0 || strcmp(line,"exit")==0) break;
        if (strcmp(line,"help")==0) { print_help(); continue; }
        if (strcmp(line,"demo")==0) { submit_demos(); continue; }

        if (strncmp(line, "submit ", 7) == 0) {
            char ts[32]={0}; char desc[TASK_DESC_LEN]={0};
            int prio=0, param=0;
            if (sscanf(line+7, "%31s %d %d %[^\n]", ts, &prio, &param, desc) < 3) {
                printf("Usage: submit <compute|io|sleep> <prio> <param> [desc]\n"); continue;
            }
            cmd.type = CMD_SUBMIT; cmd.priority = prio; cmd.param = param;
            /* mq_send() rejects priorities >= sysconf(_SC_MQ_PRIO_MAX).
             * This app uses a small 0-9 scale, so validate here and give a
             * clear message rather than letting the send fail with EINVAL. */
            if (prio < 0 || prio > 9) {
                printf("Priority must be 0-9 (9 = highest).\n"); continue;
            }
            if (param < 0) {
                printf("Parameter must be non-negative.\n"); continue;
            }
            if (strcmp(ts,"compute")==0) cmd.task_type = TASK_COMPUTE;
            else if (strcmp(ts,"io")==0) cmd.task_type = TASK_IO;
            else if (strcmp(ts,"sleep")==0) cmd.task_type = TASK_SLEEP;
            else { printf("Invalid type. Use: compute|io|sleep\n"); continue; }
            if (desc[0]) snprintf(cmd.description, TASK_DESC_LEN, "%s", desc);
            else snprintf(cmd.description, TASK_DESC_LEN, "%s(param=%d)", ts, param);
        }
        else if (strcmp(line,"list")==0)     cmd.type = CMD_LIST;
        else if (strcmp(line,"workers")==0)  cmd.type = CMD_WORKERS;
        else if (strcmp(line,"stats")==0)    cmd.type = CMD_STATS;
        else if (strcmp(line,"shutdown")==0) cmd.type = CMD_SHUTDOWN;
        else { printf("Unknown command. Type 'help'.\n"); continue; }

        if (send_cmd(&cmd, &resp) == 0) {
            printf("%s\n", resp.message);
            if (cmd.type == CMD_SHUTDOWN) break;
        }
    }
    printf("Goodbye.\n");
    return 0;
}
