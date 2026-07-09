/*
 * 02_mq_basic_receive.c — Receive a Message from a POSIX Message Queue
 *
 * Opens an existing queue and reads the first available message.
 * Run 01_mq_basic_send first to populate the queue.
 *
 * Build: gcc -o 02_mq_basic_receive 02_mq_basic_receive.c -lrt
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <mqueue.h>

#define QUEUE_NAME   "/ipc_demo_basic"
#define MAX_MSG_SIZE 256

int main(void)
{
    /* Open existing queue (read-only) */
    mqd_t mq = mq_open(QUEUE_NAME, O_RDONLY);
    if (mq == (mqd_t)-1) {
        perror("mq_open (run 01_mq_basic_send first)");
        return EXIT_FAILURE;
    }

    /* Read one message */
    char buffer[MAX_MSG_SIZE + 1];
    unsigned int priority;

    ssize_t bytes = mq_receive(mq, buffer, sizeof(buffer), &priority);
    if (bytes == -1) {
        perror("mq_receive");
        mq_close(mq);
        return EXIT_FAILURE;
    }

    buffer[bytes] = '\0';
    printf("[Receiver] Got message (priority=%u): \"%s\"\n", priority, buffer);

    /* Clean up: close and remove the queue */
    mq_close(mq);
    mq_unlink(QUEUE_NAME);
    printf("[Receiver] Queue removed.\n");

    return EXIT_SUCCESS;
}
