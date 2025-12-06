// prodcons.c
// Bounded-buffer Producer–Consumer using Pthreads + POSIX semaphores

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>

typedef int buffer_item;
#define BUFFER_SIZE 5

// Buffer and indices
buffer_item buffer[BUFFER_SIZE];
int in = 0;
int out = 0;

// Synchronization primitives
pthread_mutex_t mutex;
sem_t empty;   // counts empty slots
sem_t full;    // counts full slots

int insert_item(buffer_item item) {
    if (sem_wait(&empty) != 0) return -1;
    if (pthread_mutex_lock(&mutex) != 0) return -1;

    buffer[in] = item;
    in = (in + 1) % BUFFER_SIZE;

    if (pthread_mutex_unlock(&mutex) != 0) return -1;
    if (sem_post(&full) != 0) return -1;

    return 0;
}

int remove_item(buffer_item *item) {
    if (sem_wait(&full) != 0) return -1;
    if (pthread_mutex_lock(&mutex) != 0) return -1;

    *item = buffer[out];
    out = (out + 1) % BUFFER_SIZE;

    if (pthread_mutex_unlock(&mutex) != 0) return -1;
    if (sem_post(&empty) != 0) return -1;

    return 0;
}

void *producer(void *param) {
    long id = (long)param;
    buffer_item item;

    while (1) {
        sleep(rand() % 3 + 1);           // sleep 1–3 seconds
        item = rand() % 1000;

        if (insert_item(item) == 0) {
            printf("Producer %ld produced %d\n", id, item);
        } else {
            fprintf(stderr, "Producer %ld error inserting item\n", id);
        }
        fflush(stdout);
    }
    return NULL;
}

void *consumer(void *param) {
    long id = (long)param;
    buffer_item item;

    while (1) {
        sleep(rand() % 3 + 1);           // sleep 1–3 seconds

        if (remove_item(&item) == 0) {
            printf("Consumer %ld consumed %d\n", id, item);
        } else {
            fprintf(stderr, "Consumer %ld error removing item\n", id);
        }
        fflush(stdout);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr,
                "Usage: %s <run_time> <num_producers> <num_consumers>\n",
                argv[0]);
        exit(EXIT_FAILURE);
    }

    int run_time      = atoi(argv[1]);
    int num_producers = atoi(argv[2]);
    int num_consumers = atoi(argv[3]);

    if (run_time <= 0 || num_producers <= 0 || num_consumers <= 0) {
        fprintf(stderr, "All arguments must be positive integers.\n");
        exit(EXIT_FAILURE);
    }

    srand((unsigned int)time(NULL));

    pthread_t *producers = malloc(sizeof(pthread_t) * num_producers);
    pthread_t *consumers = malloc(sizeof(pthread_t) * num_consumers);
    if (!producers || !consumers) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    // Initialize synchronization primitives
    pthread_mutex_init(&mutex, NULL);
    sem_init(&empty, 0, BUFFER_SIZE);
    sem_init(&full, 0, 0);

    // Create producer threads
    for (long i = 0; i < num_producers; i++) {
        if (pthread_create(&producers[i], NULL, producer, (void *)i) != 0) {
            perror("pthread_create producer");
            exit(EXIT_FAILURE);
        }
    }

    // Create consumer threads
    for (long i = 0; i < num_consumers; i++) {
        if (pthread_create(&consumers[i], NULL, consumer, (void *)i) != 0) {
            perror("pthread_create consumer");
            exit(EXIT_FAILURE);
        }
    }

    // Run for the specified time
    sleep(run_time);

    // For a simple assignment, just exit; threads are terminated by process exit.
    // (If your instructor requires clean shutdown, we can add a global flag
    //  and join/cancel all threads.)
    printf("Main: run_time reached, exiting.\n");

    // Destroy synchronization primitives
    pthread_mutex_destroy(&mutex);
    sem_destroy(&empty);
    sem_destroy(&full);

    free(producers);
    free(consumers);

    return 0;
}
