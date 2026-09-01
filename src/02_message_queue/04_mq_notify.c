/*
 * 04_mq_notify.c — Async Notification on Message Arrival
 *
 * Instead of blocking in mq_receive(), this example registers with
 * mq_notify() and lets the main loop do other work until a signal
 * (SIGUSR1) announces that the queue has become readable.
 *
 * Two details from man 3 mq_notify that shape this code:
 *
 *   1. Notification fires ONLY on an empty -> non-empty transition.
 *      A message arriving at an already non-empty queue produces no
 *      signal, so the handler must drain everything that is queued.
 *
 *   2. Notification is one-shot: the registration is removed as soon
 *      as it is delivered. To keep receiving notifications you must
 *      call mq_notify() again, and the man page recommends doing so
 *      BEFORE draining the queue (as this code does), otherwise a
 *      message arriving mid-drain could be missed.
 *
 * The queue is opened O_NONBLOCK so the drain loop can empty it and
 * stop cleanly with EAGAIN instead of blocking on the last read.
 *
 * Build: gcc -o 04_mq_notify 04_mq_notify.c -lrt
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <mqueue.h>
#include <sys/wait.h>

#define QUEUE_NAME   "/ipc_demo_notify"
#define MAX_MSG_SIZE 256

static volatile sig_atomic_t msg_available = 0;
static mqd_t g_mq;

static void notify_handler(int sig)
{
    (void)sig;
    msg_available = 1;
}

/* Re-register notification (must be done after each delivery) */
static void register_notification(void)
{
    struct sigevent sev = {
        .sigev_notify = SIGEV_SIGNAL,
        .sigev_signo  = SIGUSR1,
    };
    mq_notify(g_mq, &sev);
}

int main(void)
{
    mq_unlink(QUEUE_NAME);

    struct mq_attr attr = { .mq_maxmsg = 10, .mq_msgsize = MAX_MSG_SIZE };
    g_mq = mq_open(QUEUE_NAME, O_CREAT | O_RDWR | O_NONBLOCK, 0644, &attr);
    if (g_mq == (mqd_t)-1) {
        perror("mq_open");
        return EXIT_FAILURE;
    }

    /* Set up signal handler */
    struct sigaction sa = { .sa_handler = notify_handler };
    sigemptyset(&sa.sa_mask);
    sigaction(SIGUSR1, &sa, NULL);

    /* Register for notification */
    register_notification();

    /* Fork a child that sends messages with delays */
    pid_t pid = fork();
    if (pid == 0) {
        /* --- CHILD: Send messages at intervals --- */
        const char *msgs[] = {
            "First message",
            "Second message (after 2s)",
            "Third message (after 1s)",
        };
        int delays[] = { 1, 2, 1 };

        for (int i = 0; i < 3; i++) {
            sleep(delays[i]);
            mq_send(g_mq, msgs[i], strlen(msgs[i]) + 1, 0);
            printf("[Sender] Sent: \"%s\"\n", msgs[i]);
        }
        mq_close(g_mq);
        return EXIT_SUCCESS;
    }

    /* --- PARENT: Do work, handle messages asynchronously --- */
    printf("[Main] Doing other work while waiting for messages...\n");
    printf("[Main] (Messages arrive via signal notification)\n\n");

    int received = 0;
    char buffer[MAX_MSG_SIZE + 1];

    while (received < 3) {
        if (msg_available) {
            msg_available = 0;
            /* Re-register BEFORE draining: notification is one-shot, and
             * arming it first means a message that arrives while we drain
             * still produces a notification instead of being missed. */
            register_notification();

            unsigned int prio;
            ssize_t n;
            while ((n = mq_receive(g_mq, buffer, sizeof(buffer), &prio)) > 0) {
                buffer[n] = '\0';
                received++;
                printf("[Main] Notified! Got: \"%s\"\n", buffer);
            }
        }
        /* Simulate other work */
        printf("[Main] ... working ... (tick)\n");
        sleep(1);
    }

    wait(NULL);
    mq_close(g_mq);
    mq_unlink(QUEUE_NAME);
    printf("\n[Main] All messages received asynchronously. Done.\n");

    return EXIT_SUCCESS;
}
