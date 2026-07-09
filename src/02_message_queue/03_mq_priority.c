/*
 * 03_mq_priority.c — Priority-Based Message Ordering
 *
 * Sends messages with different priorities, then receives them.
 * Messages are always dequeued highest-priority-first, regardless
 * of insertion order.
 *
 * Build: gcc -o 03_mq_priority 03_mq_priority.c -lrt
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mqueue.h>

#define QUEUE_NAME   "/ipc_demo_priority"
#define MAX_MSG_SIZE 256
#define MAX_MSGS     10

int main(void)
{
    mq_unlink(QUEUE_NAME); /* Clean start */

    struct mq_attr attr = {
        .mq_maxmsg  = MAX_MSGS,
        .mq_msgsize = MAX_MSG_SIZE
    };

    mqd_t mq = mq_open(QUEUE_NAME, O_CREAT | O_RDWR, 0644, &attr);
    if (mq == (mqd_t)-1) {
        perror("mq_open");
        return EXIT_FAILURE;
    }

    /* Send messages in non-priority order */
    struct { const char *text; unsigned int prio; } msgs[] = {
        { "Low priority: batch report",      1 },
        { "Normal: user data update",        3 },
        { "High priority: payment processing", 7 },
        { "Critical: system alert!",         9 },
        { "Normal: log rotation",            3 },
    };
    int count = sizeof(msgs) / sizeof(msgs[0]);

    printf("=== Sending %d messages (mixed priority order) ===\n\n", count);
    for (int i = 0; i < count; i++) {
        mq_send(mq, msgs[i].text, strlen(msgs[i].text) + 1, msgs[i].prio);
        printf("  Sent [prio=%u]: \"%s\"\n", msgs[i].prio, msgs[i].text);
    }

    /* Now receive — highest priority comes first */
    printf("\n=== Receiving (highest priority first) ===\n\n");

    char buffer[MAX_MSG_SIZE + 1];
    unsigned int prio;
    struct mq_attr current;
    mq_getattr(mq, &current);

    for (int i = 0; i < count; i++) {
        ssize_t n = mq_receive(mq, buffer, sizeof(buffer), &prio);
        if (n > 0) {
            buffer[n] = '\0';
            printf("  Received #%d [prio=%u]: \"%s\"\n", i + 1, prio, buffer);
        }
    }

    mq_close(mq);
    mq_unlink(QUEUE_NAME);
    printf("\n[Done] Notice: messages came out in priority order, not insertion order.\n");

    return EXIT_SUCCESS;
}
