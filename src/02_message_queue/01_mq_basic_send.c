/*
 * 01_mq_basic_send.c — Send a Message to a POSIX Message Queue
 *
 * Creates a message queue and sends a single message.
 * Run 02_mq_basic_receive to read it.
 *
 * The queue persists in the kernel until explicitly unlinked.
 * You can see it at: /dev/mqueue/ipc_demo_basic
 *
 * Build: gcc -o 01_mq_basic_send 01_mq_basic_send.c -lrt
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mqueue.h>

#define QUEUE_NAME  "/ipc_demo_basic"
#define MAX_MSG_SIZE 256
#define MAX_MSGS     10

int main(void)
{
    /* Configure queue attributes */
    struct mq_attr attr = {
        .mq_flags   = 0,
        .mq_maxmsg  = MAX_MSGS,     /* Max messages in queue */
        .mq_msgsize = MAX_MSG_SIZE,  /* Max size of each message */
        .mq_curmsgs = 0
    };

    /* Create or open the queue */
    mqd_t mq = mq_open(QUEUE_NAME, O_CREAT | O_WRONLY, 0644, &attr);
    if (mq == (mqd_t)-1) {
        perror("mq_open");
        return EXIT_FAILURE;
    }

    /* Send a message with priority 0 */
    const char *msg = "Hello from message queue!";
    printf("[Sender] Queue opened: %s\n", QUEUE_NAME);
    printf("[Sender] Sending: \"%s\"\n", msg);

    if (mq_send(mq, msg, strlen(msg) + 1, 0) == -1) {
        perror("mq_send");
        mq_close(mq);
        return EXIT_FAILURE;
    }

    printf("[Sender] Message sent successfully.\n");
    printf("[Sender] Run 02_mq_basic_receive to read it.\n");

    mq_close(mq);
    return EXIT_SUCCESS;
}
