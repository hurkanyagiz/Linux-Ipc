/*
 * task_worker.c — Task Executor (Capstone Project)
 *
 * IPC methods used:
 *   Message Queue — Receive tasks from dispatcher
 *   Shared Memory — Write task results and worker status
 *   Semaphore    — Protect shared memory access
 *   Signal       — SIGUSR1 (new task), SIGTERM (shutdown)
 *
 * Usage: ./task_worker <worker_id>
 * Build: gcc -o task_worker task_worker.c -lrt -lpthread -lm
 */
#define _GNU_SOURCE
#include "../include/task_manager.h"

static volatile sig_atomic_t g_shutdown = 0;
static volatile sig_atomic_t g_new_task = 0;
static shared_state_t *g_state = NULL;
static sem_t          *g_sem   = NULL;
static mqd_t           g_mq   = (mqd_t)-1;
static int             g_wid  = 0;

static void on_usr1(int s) { (void)s; g_new_task = 1; }
static void on_term(int s) { (void)s; g_shutdown = 1; }

static int connect_ipc(void) {
    int fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (fd == -1) { perror("shm_open"); return -1; }
    g_state = mmap(NULL, sizeof(shared_state_t), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    if (g_state == MAP_FAILED) return -1;
    g_sem = sem_open(SEM_NAME, 0);
    if (g_sem == SEM_FAILED) return -1;
    g_mq = mq_open(MQ_NAME, O_RDONLY);
    if (g_mq == (mqd_t)-1) return -1;
    return 0;
}

static void register_worker(void) {
    sem_wait(g_sem);
    int i = g_state->worker_count;
    if (i < MAX_WORKERS) {
        g_state->workers[i].worker_id = g_wid;
        g_state->workers[i].pid = getpid();
        g_state->workers[i].active = 1;
        g_state->workers[i].last_heartbeat = time(NULL);
        g_state->worker_count++;
    }
    sem_post(g_sem);
}

static void unregister_worker(void) {
    sem_wait(g_sem);
    for (int i = 0; i < g_state->worker_count; i++)
        if (g_state->workers[i].worker_id == g_wid)
            g_state->workers[i].active = 0;
    sem_post(g_sem);
}

static void update_task(int id, task_status_t status, int result, const char *text) {
    sem_wait(g_sem);
    for (int i = 0; i < g_state->task_count; i++) {
        if (g_state->tasks[i].task_id == id) {
            g_state->tasks[i].status = status;
            g_state->tasks[i].worker_id = g_wid;
            g_state->tasks[i].result = result;
            if (text) strncpy(g_state->tasks[i].result_text, text, RESULT_LEN-1);
            if (status >= TASK_COMPLETED) g_state->tasks[i].completed_at = time(NULL);
            break;
        }
    }
    for (int i = 0; i < g_state->worker_count; i++) {
        if (g_state->workers[i].worker_id == g_wid) {
            g_state->workers[i].last_heartbeat = time(NULL);
            if (status == TASK_COMPLETED) { g_state->workers[i].tasks_completed++; g_state->total_completed++; }
            if (status == TASK_FAILED)    { g_state->workers[i].tasks_failed++;    g_state->total_failed++; }
            break;
        }
    }
    sem_post(g_sem);
}

static void process_task(task_msg_t *msg) {
    printf("[Worker-%d] === Task #%d: \"%s\" ===\n", g_wid, msg->task_id, msg->description);
    update_task(msg->task_id, TASK_RUNNING, 0, "Processing...");

    int result = 0;
    char text[RESULT_LEN];

    switch (msg->type) {
    case TASK_COMPUTE: {
        int n = msg->param, a = 0, b = 1;
        for (int i = 2; i <= n && !g_shutdown; i++) { int c = a+b; a = b; b = c; usleep(100000); }
        result = (n <= 1) ? n : b;
        snprintf(text, RESULT_LEN, "fibonacci(%d) = %d", n, result);
        break;
    }
    case TASK_IO:
        for (int i = 0; i < msg->param && !g_shutdown; i++) { usleep(200000); result += (i+1)*10; }
        snprintf(text, RESULT_LEN, "IO completed: %d units", result);
        break;
    case TASK_SLEEP:
        for (int i = 0; i < msg->param && !g_shutdown; i++) sleep(1);
        result = msg->param;
        snprintf(text, RESULT_LEN, "Slept %d seconds", result);
        break;
    default:
        snprintf(text, RESULT_LEN, "Unknown task type");
        update_task(msg->task_id, TASK_FAILED, 0, text);
        return;
    }

    update_task(msg->task_id, TASK_COMPLETED, result, text);
    printf("[Worker-%d] Done: %s\n", g_wid, text);
}

int main(int argc, char *argv[]) {
    if (argc != 2) { fprintf(stderr, "Usage: %s <worker_id>\n", argv[0]); return 1; }
    g_wid = atoi(argv[1]);

    printf("[Worker-%d] Starting (PID=%d)...\n", g_wid, getpid());

    struct sigaction sa = {.sa_handler = on_usr1}; sigemptyset(&sa.sa_mask);
    sigaction(SIGUSR1, &sa, NULL);
    sa.sa_handler = on_term;
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);

    if (connect_ipc() == -1) {
        fprintf(stderr, "[Worker-%d] Cannot connect IPC. Is dispatcher running?\n", g_wid);
        return 1;
    }
    register_worker();
    printf("[Worker-%d] Registered. Waiting for tasks...\n\n", g_wid);

    task_msg_t msg;
    while (!g_shutdown) {
        sem_wait(g_sem);
        int alive = g_state->dispatcher_running;
        sem_post(g_sem);
        if (!alive) break;

        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += 2;
        unsigned int prio;
        ssize_t n = mq_timedreceive(g_mq, (char *)&msg, sizeof(msg), &prio, &ts);
        if (n == (ssize_t)sizeof(msg)) process_task(&msg);
        g_new_task = 0;
    }

    printf("\n[Worker-%d] Shutting down...\n", g_wid);
    unregister_worker();
    mq_close(g_mq); sem_close(g_sem);
    munmap(g_state, sizeof(shared_state_t));
    printf("[Worker-%d] Goodbye.\n", g_wid);
    return 0;
}
