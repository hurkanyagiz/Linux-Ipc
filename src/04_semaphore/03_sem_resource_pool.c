/*
 * 03_sem_resource_pool.c — Counting Semaphore as a Resource Pool
 *
 * Simulates a database connection pool with limited capacity.
 * A counting semaphore limits concurrent access to N connections.
 * Workers must acquire a "connection" before doing work.
 *
 * This pattern is used in web servers, database drivers, and
 * thread pool implementations.
 *
 * Build: gcc -o 03_sem_resource_pool 03_sem_resource_pool.c -lpthread
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>

#define POOL_SIZE    3   /* Only 3 connections available */
#define NUM_WORKERS  8   /* 8 workers competing for connections */

static sem_t pool_sem;
static sem_t print_mutex;

static void safe_print(const char *fmt, int worker_id, int pool_avail)
{
    sem_wait(&print_mutex);
    printf(fmt, worker_id, pool_avail);
    sem_post(&print_mutex);
}

static void *worker(void *arg)
{
    int id = *(int *)arg;
    int avail;

    sem_getvalue(&pool_sem, &avail);
    safe_print("[Worker-%d] Requesting connection... (%d available)\n", id, avail);

    /* Acquire a connection (blocks if pool is exhausted) */
    sem_wait(&pool_sem);

    sem_getvalue(&pool_sem, &avail);
    safe_print("[Worker-%d] Got connection! (%d remaining)\n", id, avail);

    /* Simulate database work */
    usleep((rand() % 500 + 300) * 1000);

    /* Release the connection */
    sem_post(&pool_sem);

    sem_getvalue(&pool_sem, &avail);
    safe_print("[Worker-%d] Released connection. (%d available)\n", id, avail);

    return NULL;
}

int main(void)
{
    srand(time(NULL));

    printf("=== Connection Pool with Counting Semaphore ===\n");
    printf("  Pool size: %d connections\n", POOL_SIZE);
    printf("  Workers: %d (competing for connections)\n\n", NUM_WORKERS);

    sem_init(&pool_sem, 0, POOL_SIZE);
    sem_init(&print_mutex, 0, 1);

    pthread_t threads[NUM_WORKERS];
    int ids[NUM_WORKERS];

    for (int i = 0; i < NUM_WORKERS; i++) {
        ids[i] = i + 1;
        pthread_create(&threads[i], NULL, worker, &ids[i]);
        usleep(50000); /* Stagger start slightly */
    }

    for (int i = 0; i < NUM_WORKERS; i++)
        pthread_join(threads[i], NULL);

    sem_destroy(&pool_sem);
    sem_destroy(&print_mutex);

    printf("\n[Done] All workers completed. Max %d concurrent connections.\n",
           POOL_SIZE);

    return EXIT_SUCCESS;
}
