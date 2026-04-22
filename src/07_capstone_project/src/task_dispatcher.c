/*
 * task_dispatcher.c — Central Coordinator (Capstone Project)
 *
 * IPC methods used:
 *   Socket       — Accept client commands
 *   Message Queue — Distribute tasks to workers (priority-ordered)
 *   Shared Memory — Store task/worker status table
 *   Semaphore    — Protect shared memory access
 *   Signal       — Notify workers (SIGUSR1), shutdown (SIGTERM)
 *
 * Build: gcc -o task_dispatcher task_dispatcher.c -lrt -lpthread
 */
#define _GNU_SOURCE
#include "../include/task_manager.h"

static volatile sig_atomic_t g_shutdown = 0;
static shared_state_t *g_state = NULL;
static sem_t          *g_sem   = NULL;
static mqd_t           g_mq   = (mqd_t)-1;

static void shutdown_handler(int s) { (void)s; g_shutdown = 1; }

/* ── IPC initialization ── */

static int init_shm(void) {
    int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (fd == -1) { perror("shm_open"); return -1; }
    ftruncate(fd, sizeof(shared_state_t));
    g_state = mmap(NULL, sizeof(shared_state_t),
                   PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    if (g_state == MAP_FAILED) { perror("mmap"); return -1; }
    memset(g_state, 0, sizeof(shared_state_t));
    g_state->dispatcher_running = 1;
    g_state->dispatcher_pid = getpid();
    g_state->next_task_id = 1;
    return 0;
}

static int init_sem(void) {
    sem_unlink(SEM_NAME);
    g_sem = sem_open(SEM_NAME, O_CREAT, 0666, 1);
    if (g_sem == SEM_FAILED) { perror("sem_open"); return -1; }
    return 0;
}

static int init_mq(void) {
    mq_unlink(MQ_NAME);
    struct mq_attr attr = { .mq_maxmsg = MQ_MAX_MSGS, .mq_msgsize = MQ_MSG_SIZE };
    g_mq = mq_open(MQ_NAME, O_CREAT | O_WRONLY, 0666, &attr);
    if (g_mq == (mqd_t)-1) { perror("mq_open"); return -1; }
    return 0;
}

/* ── Task operations ── */

static int submit_task(task_type_t type, int priority, int param, const char *desc) {
    sem_wait(g_sem);
    if (g_state->task_count >= MAX_TASKS) { sem_post(g_sem); return -1; }
    int id = g_state->next_task_id++;
    int idx = g_state->task_count++;
    task_entry_t *e = &g_state->tasks[idx];
    e->task_id = id;
    e->status = TASK_PENDING;
    e->worker_id = -1;
    e->created_at = time(NULL);
    strncpy(e->description, desc, TASK_DESC_LEN - 1);
    snprintf(e->result_text, RESULT_LEN, "Pending...");
    g_state->total_submitted++;

    /* Notify active workers via SIGUSR1 */
    for (int i = 0; i < g_state->worker_count; i++)
        if (g_state->workers[i].active && g_state->workers[i].pid > 0)
            kill(g_state->workers[i].pid, SIGUSR1);
    sem_post(g_sem);

    /* Enqueue via message queue */
    task_msg_t msg = { .task_id = id, .type = type, .priority = priority, .param = param };
    strncpy(msg.description, desc, TASK_DESC_LEN - 1);
    if (mq_send(g_mq, (char *)&msg, sizeof(msg), priority) == -1) {
        perror("mq_send"); return -1;
    }
    return id;
}

static void build_list(char *buf, size_t len) {
    sem_wait(g_sem);
    int off = snprintf(buf, len, "\n%-4s %-10s %-30s %-11s %-8s\n",
                       "ID", "Type", "Description", "Status", "Worker");
    off += snprintf(buf+off, len-off, "%-4s %-10s %-30s %-11s %-8s\n",
                    "----", "----------", "------------------------------",
                    "-----------", "--------");
    for (int i = 0; i < g_state->task_count && off < (int)len-200; i++) {
        task_entry_t *t = &g_state->tasks[i];
        char w[8]; snprintf(w, sizeof(w), t->worker_id >= 0 ? "W-%d" : "-", t->worker_id);
        off += snprintf(buf+off, len-off, "%-4d %-10s %-30.30s %-11s %-8s\n",
                        t->task_id, "-",
                        t->description, task_status_str[t->status], w);
    }
    if (g_state->task_count == 0)
        off += snprintf(buf+off, len-off, "(no tasks)\n");
    sem_post(g_sem);
}

static void build_stats(char *buf, size_t len) {
    sem_wait(g_sem);
    snprintf(buf, len,
        "\n--- System Statistics ---\n"
        "  Total submitted : %d\n"
        "  Completed       : %d\n"
        "  Failed          : %d\n"
        "  Pending         : %d\n"
        "  Active workers  : %d\n"
        "  Dispatcher PID  : %d\n",
        g_state->total_submitted, g_state->total_completed, g_state->total_failed,
        g_state->total_submitted - g_state->total_completed - g_state->total_failed,
        g_state->worker_count, g_state->dispatcher_pid);
    sem_post(g_sem);
}

static void build_workers(char *buf, size_t len) {
    sem_wait(g_sem);
    int off = snprintf(buf, len, "\n--- Worker Status ---\n");
    for (int i = 0; i < g_state->worker_count && off < (int)len-200; i++) {
        worker_info_t *w = &g_state->workers[i];
        char hb[32]; timestamp_str(w->last_heartbeat, hb, sizeof(hb));
        off += snprintf(buf+off, len-off,
            "  Worker-%d  PID=%-6d Active=%-3s Done=%d Failed=%d Last=%s\n",
            w->worker_id, w->pid, w->active?"Yes":"No",
            w->tasks_completed, w->tasks_failed, hb);
    }
    if (g_state->worker_count == 0)
        snprintf(buf+off, len-off, "  (no workers registered)\n");
    sem_post(g_sem);
}

/* ── Client handling (Socket) ── */

static void handle_client(int cfd) {
    client_cmd_t cmd;
    server_response_t resp;
    if (recv(cfd, &cmd, sizeof(cmd), 0) != sizeof(cmd)) { close(cfd); return; }
    memset(&resp, 0, sizeof(resp));
    switch (cmd.type) {
    case CMD_SUBMIT: {
        int id = submit_task(cmd.task_type, cmd.priority, cmd.param, cmd.description);
        resp.success = (id > 0);
        snprintf(resp.message, sizeof(resp.message),
                 id > 0 ? "Task #%d created [%s] prio=%d" : "Failed to create task",
                 id, task_type_str(cmd.task_type), cmd.priority);
        break;
    }
    case CMD_LIST:     resp.success=1; build_list(resp.message, sizeof(resp.message)); break;
    case CMD_STATS:    resp.success=1; build_stats(resp.message, sizeof(resp.message)); break;
    case CMD_WORKERS:  resp.success=1; build_workers(resp.message, sizeof(resp.message)); break;
    case CMD_SHUTDOWN: resp.success=1; snprintf(resp.message,sizeof(resp.message),"Shutting down..."); g_shutdown=1; break;
    default: snprintf(resp.message, sizeof(resp.message), "Unknown command: %d", cmd.type);
    }
    send(cfd, &resp, sizeof(resp), 0);
    close(cfd);
}

/* ── Main ── */

int main(void) {
    printf("=== Distributed Task Manager — Dispatcher ===\n");
    printf("PID: %d\n\n", getpid());

    struct sigaction sa = {.sa_handler = shutdown_handler};
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    signal(SIGPIPE, SIG_IGN);

    if (init_shm() == -1 || init_sem() == -1 || init_mq() == -1)
        return EXIT_FAILURE;

    printf("[Dispatcher] Shared memory : %s\n", SHM_NAME);
    printf("[Dispatcher] Semaphore     : %s\n", SEM_NAME);
    printf("[Dispatcher] Message queue : %s\n", MQ_NAME);

    unlink(SOCKET_PATH);
    int sfd = socket(AF_UNIX, SOCK_STREAM, 0);
    struct sockaddr_un addr = {.sun_family = AF_UNIX};
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path)-1);
    bind(sfd, (struct sockaddr *)&addr, sizeof(addr));
    listen(sfd, 5);
    printf("[Dispatcher] Socket        : %s\n", SOCKET_PATH);
    printf("\n[Dispatcher] Ready. Start workers and client.\n\n");

    struct timeval tv;
    while (!g_shutdown) {
        fd_set fds; FD_ZERO(&fds); FD_SET(sfd, &fds);
        tv.tv_sec = 1; tv.tv_usec = 0;
        if (select(sfd+1, &fds, NULL, NULL, &tv) > 0 && FD_ISSET(sfd, &fds)) {
            int cfd = accept(sfd, NULL, NULL);
            if (cfd >= 0) handle_client(cfd);
        }
    }

    printf("\n[Dispatcher] Shutting down...\n");
    sem_wait(g_sem);
    g_state->dispatcher_running = 0;
    for (int i = 0; i < g_state->worker_count; i++)
        if (g_state->workers[i].active && g_state->workers[i].pid > 0)
            kill(g_state->workers[i].pid, SIGTERM);
    sem_post(g_sem);
    sleep(1);

    close(sfd); unlink(SOCKET_PATH);
    mq_close(g_mq); mq_unlink(MQ_NAME);
    sem_close(g_sem); sem_unlink(SEM_NAME);
    munmap(g_state, sizeof(shared_state_t)); shm_unlink(SHM_NAME);
    printf("[Dispatcher] All IPC resources cleaned. Goodbye.\n");
    return 0;
}
