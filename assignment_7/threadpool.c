// threadpool.c
// Simple fixed-size thread pool using Pthreads + condition variables

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

typedef struct {
    void (*function)(void *);
    void *argument;
} task_t;

#define MAX_QUEUE 64

typedef struct {
    pthread_t *threads;
    int num_threads;

    task_t queue[MAX_QUEUE];
    int queue_front;
    int queue_rear;
    int queue_size;

    pthread_mutex_t lock;
    pthread_cond_t notify;

    int shutdown; // 0 = running, 1 = shutting down
} threadpool_t;

static threadpool_t pool;

void *worker_thread(void *arg) {
    (void)arg;
    while (1) {
        pthread_mutex_lock(&pool.lock);

        // Wait for a task
        while (pool.queue_size == 0 && !pool.shutdown) {
            pthread_cond_wait(&pool.notify, &pool.lock);
        }

        // If shutting down and no work left, exit
        if (pool.shutdown && pool.queue_size == 0) {
            pthread_mutex_unlock(&pool.lock);
            break;
        }

        // Get next task
        task_t task = pool.queue[pool.queue_front];
        pool.queue_front = (pool.queue_front + 1) % MAX_QUEUE;
        pool.queue_size--;

        pthread_mutex_unlock(&pool.lock);

        // Execute task outside lock
        (*(task.function))(task.argument);
    }

    return NULL;
}

// Initialize the thread pool with n threads
void pool_init(int n) {
    if (n <= 0 || n > 64) {
        fprintf(stderr, "pool_init: invalid number of threads\n");
        exit(EXIT_FAILURE);
    }

    pool.num_threads = n;
    pool.queue_front = 0;
    pool.queue_rear = 0;
    pool.queue_size = 0;
    pool.shutdown = 0;

    pthread_mutex_init(&pool.lock, NULL);
    pthread_cond_init(&pool.notify, NULL);

    pool.threads = malloc(sizeof(pthread_t) * n);
    if (!pool.threads) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < n; i++) {
        if (pthread_create(&pool.threads[i], NULL, worker_thread, NULL) != 0) {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }
    }
}

// Submit a task to the pool
// Returns 0 on success, -1 if queue is full or shutting down
int pool_submit(void (*somefunction)(void *), void *p) {
    pthread_mutex_lock(&pool.lock);

    if (pool.shutdown || pool.queue_size == MAX_QUEUE) {
        pthread_mutex_unlock(&pool.lock);
        return -1;
    }

    pool.queue[pool.queue_rear].function = somefunction;
    pool.queue[pool.queue_rear].argument = p;
    pool.queue_rear = (pool.queue_rear + 1) % MAX_QUEUE;
    pool.queue_size++;

    pthread_cond_signal(&pool.notify);
    pthread_mutex_unlock(&pool.lock);

    return 0;
}

// Shut down the pool and wait for worker threads to finish
void pool_shutdown(void) {
    pthread_mutex_lock(&pool.lock);
    pool.shutdown = 1;
    pthread_cond_broadcast(&pool.notify);
    pthread_mutex_unlock(&pool.lock);

    for (int i = 0; i < pool.num_threads; i++) {
        pthread_join(pool.threads[i], NULL);
    }

    pthread_mutex_destroy(&pool.lock);
    pthread_cond_destroy(&pool.notify);
    free(pool.threads);
}

// ------------------------------------------------------------
// Example client code using the pool
// ------------------------------------------------------------

void example_task(void *arg) {
    int id = *(int *)arg;
    printf("Task %d: running in thread %lu\n", id, (unsigned long)pthread_self());
    fflush(stdout);
    sleep(1);   // simulate some work
}

int main(int argc, char *argv[]) {
    int num_threads = 4;
    int num_tasks = 10;

    if (argc >= 2) num_threads = atoi(argv[1]);
    if (argc >= 3) num_tasks   = atoi(argv[2]);

    printf("Initializing thread pool with %d threads\n", num_threads);
    pool_init(num_threads);

    int *task_ids = malloc(sizeof(int) * num_tasks);
    if (!task_ids) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < num_tasks; i++) {
        task_ids[i] = i;
        if (pool_submit(example_task, &task_ids[i]) != 0) {
            fprintf(stderr, "Failed to submit task %d (queue full?)\n", i);
        }
    }

    // Give some time for all tasks to be processed
    sleep(5);

    printf("Shutting down pool\n");
    pool_shutdown();

    free(task_ids);
    return 0;
}
