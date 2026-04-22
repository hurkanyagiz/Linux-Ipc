/*
 * task_manager.h — Distributed Task Manager: Shared Header
 *
 * Common data structures, constants, and types used by all components
 * (dispatcher, worker, client) of the capstone project.
 */

#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/un.h>
#include <mqueue.h>
#include <semaphore.h>
#include <pthread.h>

/* ── Configuration ── */
#define MAX_WORKERS       4
#define MAX_TASKS         64
#define TASK_DESC_LEN     128
#define RESULT_LEN        256

#define SOCKET_PATH       "/tmp/task_manager.sock"
#define MQ_NAME           "/task_manager_queue"
#define SHM_NAME          "/task_manager_shm"
#define SEM_NAME          "/task_manager_sem"

#define MQ_MAX_MSGS       10
#define MQ_MSG_SIZE       sizeof(task_msg_t)
#define CMD_BUFFER_SIZE   1024

/* ── Task status ── */
typedef enum {
    TASK_PENDING = 0, TASK_ASSIGNED, TASK_RUNNING, TASK_COMPLETED, TASK_FAILED
} task_status_t;

static const char *task_status_str[] = {
    "PENDING", "ASSIGNED", "RUNNING", "COMPLETED", "FAILED"
};

/* ── Task types ── */
typedef enum {
    TASK_COMPUTE = 1,  /* CPU-bound work */
    TASK_IO      = 2,  /* I/O simulation */
    TASK_SLEEP   = 3,  /* Timed wait */
} task_type_t;

/* ── Message queue message ── */
typedef struct {
    int           task_id;
    task_type_t   type;
    int           priority;
    int           param;
    char          description[TASK_DESC_LEN];
} task_msg_t;

/* ── Shared memory structures ── */
typedef struct {
    int           task_id;
    task_status_t status;
    int           worker_id;
    int           result;
    char          description[TASK_DESC_LEN];
    char          result_text[RESULT_LEN];
    time_t        created_at;
    time_t        completed_at;
} task_entry_t;

typedef struct {
    int           worker_id;
    pid_t         pid;
    int           active;
    int           tasks_completed;
    int           tasks_failed;
    time_t        last_heartbeat;
} worker_info_t;

typedef struct {
    task_entry_t  tasks[MAX_TASKS];
    int           task_count;
    int           next_task_id;
    worker_info_t workers[MAX_WORKERS];
    int           worker_count;
    int           total_submitted;
    int           total_completed;
    int           total_failed;
    int           dispatcher_running;
    pid_t         dispatcher_pid;
} shared_state_t;

/* ── Socket command protocol ── */
typedef enum {
    CMD_SUBMIT = 1, CMD_STATUS, CMD_LIST, CMD_WORKERS, CMD_STATS, CMD_SHUTDOWN = 9
} cmd_type_t;

typedef struct {
    cmd_type_t  type;
    int         task_type;
    int         priority;
    int         param;
    int         task_id;
    char        description[TASK_DESC_LEN];
} client_cmd_t;

typedef struct {
    int         success;
    char        message[CMD_BUFFER_SIZE];
} server_response_t;

/* ── Helpers ── */
static inline const char *task_type_str(task_type_t t) {
    switch (t) {
    case TASK_COMPUTE: return "COMPUTE";
    case TASK_IO:      return "IO";
    case TASK_SLEEP:   return "SLEEP";
    default:           return "UNKNOWN";
    }
}

static inline void timestamp_str(time_t t, char *buf, size_t len) {
    if (t == 0) { snprintf(buf, len, "-"); return; }
    struct tm *tm = localtime(&t);
    strftime(buf, len, "%H:%M:%S", tm);
}

#endif
