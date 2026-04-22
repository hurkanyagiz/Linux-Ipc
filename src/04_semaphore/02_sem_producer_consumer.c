/*
 * 02_sem_producer_consumer.c — Classic Producer-Consumer Problem
 *
 * Multiple producer threads produce items into a bounded buffer.
 * Multiple consumer threads consume items from the buffer.
 * Three semaphores coordinate access:
 *   - sem_empty: counts free slots (initially N)
 *   - sem_full:  counts filled slots (initially 0)
 *   - sem_mutex: protects buffer access (binary, initially 1)
 *
 * Build: gcc -o 02_sem_producer_consumer 02_sem_producer_consumer.c -lpthread
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>

#define BUFFER_SIZE    5
#define NUM_PRODUCERS  2
#define NUM_CONSUMERS  2
#define ITEMS_PER_PROD 5

/* Circular buffer */
static int buffer[BUFFER_SIZE];
static int in  = 0;  /* Write index */
static int out = 0;  /* Read index */

/* Semaphores */
static sem_t sem_empty;
static sem_t sem_full;
static sem_t sem_mutex;

static int total_produced = 0;
static int total_consumed = 0;

static void print_buffer(int count)
{
    printf("  Buffer [");
    for (int i = 0; i < BUFFER_SIZE; i++) {
        int idx = (out + i) % BUFFER_SIZE;
        if (i < count)
            printf(" %3d", buffer[idx]);
        else
            printf("   _");
    }
    printf(" ] (%d/%d)\n", count, BUFFER_SIZE);
}

static void *producer(void *arg)
{
    int id = *(int *)arg;

    for (int i = 0; i < ITEMS_PER_PROD; i++) {
        int item = id * 100 + i;

        sem_wait(&sem_empty); /* Wait for a free slot */
        sem_wait(&sem_mutex); /* Lock buffer access */

        /* --- Critical section: add item --- */
        buffer[in] = item;
        in = (in + 1) % BUFFER_SIZE;
        total_produced++;

        int count = 0;
        sem_getvalue(&sem_full, &count);
        count++; /* This item hasn't been posted yet */
        printf("[Producer-%d] Produced: %d\n", id, item);
        print_buffer(count);

        sem_post(&sem_mutex); /* Unlock */
        sem_post(&sem_full);  /* Signal: one more item available */

        usleep((rand() % 200 + 100) * 1000);
    }
    return NULL;
}

static void *consumer(void *arg)
{
    int id = *(int *)arg;
    int total_expected = NUM_PRODUCERS * ITEMS_PER_PROD;

    while (1) {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += 2;

        if (sem_timedwait(&sem_full, &ts) == -1) {
            sem_wait(&sem_mutex);
            int done = (total_consumed >= total_expected);
            sem_post(&sem_mutex);
            if (done) break;
            continue;
        }

        sem_wait(&sem_mutex);

        int item = buffer[out];
        out = (out + 1) % BUFFER_SIZE;
        total_consumed++;

        int count = 0;
        sem_getvalue(&sem_full, &count);
        printf("[Consumer-%d] Consumed: %d\n", id, item);
        print_buffer(count);

        int done = (total_consumed >= total_expected);
        sem_post(&sem_mutex);
        sem_post(&sem_empty);

        if (done) break;
        usleep((rand() % 300 + 200) * 1000);
    }
    return NULL;
}

int main(void)
{
    srand(time(NULL));

    printf("=== Producer-Consumer with Semaphores ===\n");
    printf("  Buffer size: %d | Producers: %d | Consumers: %d\n",
           BUFFER_SIZE, NUM_PRODUCERS, NUM_CONSUMERS);
    printf("  Each producer creates %d items\n\n", ITEMS_PER_PROD);

    sem_init(&sem_empty, 0, BUFFER_SIZE);
    sem_init(&sem_full,  0, 0);
    sem_init(&sem_mutex, 0, 1);

    pthread_t prods[NUM_PRODUCERS], cons[NUM_CONSUMERS];
    int prod_ids[NUM_PRODUCERS], cons_ids[NUM_CONSUMERS];

    for (int i = 0; i < NUM_PRODUCERS; i++) {
        prod_ids[i] = i + 1;
        pthread_create(&prods[i], NULL, producer, &prod_ids[i]);
    }
    for (int i = 0; i < NUM_CONSUMERS; i++) {
        cons_ids[i] = i + 1;
        pthread_create(&cons[i], NULL, consumer, &cons_ids[i]);
    }

    for (int i = 0; i < NUM_PRODUCERS; i++) pthread_join(prods[i], NULL);
    for (int i = 0; i < NUM_CONSUMERS; i++) pthread_join(cons[i], NULL);

    printf("\n=== Result: %d produced, %d consumed ===\n",
           total_produced, total_consumed);

    sem_destroy(&sem_empty);
    sem_destroy(&sem_full);
    sem_destroy(&sem_mutex);

    return EXIT_SUCCESS;
}
