/*
 * 03_timer_alarm.c — Periodic Timer Using SIGALRM
 *
 * Uses alarm() and setitimer() to create periodic events.
 * SIGALRM is delivered when the timer expires.
 *
 * Build: gcc -o 03_timer_alarm 03_timer_alarm.c
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>

static volatile sig_atomic_t alarm_count = 0;
static volatile sig_atomic_t running = 1;

static void alarm_handler(int s) {
    (void)s;
    alarm_count++;
    if (alarm_count >= 5) running = 0;
}

int main(void) {
    struct sigaction sa = {.sa_handler = alarm_handler};
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGALRM, &sa, NULL);

    printf("=== Periodic Timer with SIGALRM ===\n\n");

    /* Set up a repeating timer: fires every 1 second */
    struct itimerval timer = {
        .it_value    = {.tv_sec = 1, .tv_usec = 0}, /* First fire after 1s */
        .it_interval = {.tv_sec = 1, .tv_usec = 0}, /* Repeat every 1s */
    };
    setitimer(ITIMER_REAL, &timer, NULL);

    printf("[Timer] Started. Will fire 5 times, once per second.\n\n");
    int prev_count = 0;
    while (running) {
        if (alarm_count != prev_count) {
            time_t now = time(NULL);
            struct tm *t = localtime(&now);
            printf("[Timer] Alarm #%d fired at %02d:%02d:%02d\n",
                   alarm_count, t->tm_hour, t->tm_min, t->tm_sec);
            prev_count = alarm_count;
        }
        usleep(50000);
    }

    /* Cancel the timer */
    struct itimerval zero = {{0,0},{0,0}};
    setitimer(ITIMER_REAL, &zero, NULL);

    printf("\n[Timer] 5 alarms completed. Timer cancelled.\n");
    return 0;
}
