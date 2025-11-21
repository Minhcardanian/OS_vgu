#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

typedef struct {
    long start;
    long *seq;
    size_t length;
    size_t capacity;
} collatz_data_t;

void *collatz_worker(void *arg) {
    collatz_data_t *data = (collatz_data_t *)arg;
    long n = data->start;
    size_t i = 0;

    while (1) {
        if (i >= data->capacity) {
            size_t new_cap = data->capacity * 2;
            long *new_seq = realloc(data->seq, new_cap * sizeof(long));
            if (new_seq == NULL) {
                perror("realloc");
                pthread_exit(NULL);
            }
            data->seq = new_seq;
            data->capacity = new_cap;
        }

        data->seq[i++] = n;

        if (n == 1) break;
        else if (n % 2 == 0) n = n / 2;
        else n = 3 * n + 1;
    }

    data->length = i;
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <positive integer>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *endptr;
    long start = strtol(argv[1], &endptr, 10);
    if (*endptr != '\0' || start <= 0) {
        fprintf(stderr, "Error: argument must be a positive integer\n");
        return EXIT_FAILURE;
    }

    collatz_data_t data;
    data.start = start;
    data.capacity = 64;
    data.length = 0;
    data.seq = malloc(data.capacity * sizeof(long));
    if (data.seq == NULL) {
        perror("malloc");
        return EXIT_FAILURE;
    }

    pthread_t tid;
    if (pthread_create(&tid, NULL, collatz_worker, &data) != 0) {
        perror("pthread_create");
        free(data.seq);
        return EXIT_FAILURE;
    }

    if (pthread_join(tid, NULL) != 0) {
        perror("pthread_join");
        free(data.seq);
        return EXIT_FAILURE;
    }

    printf("Collatz sequence for %ld:\n", start);
    for (size_t i = 0; i < data.length; i++) {
        printf("%ld%s", data.seq[i], (i + 1 < data.length) ? " " : "");
    }
    printf("\n");

    free(data.seq);
    return EXIT_SUCCESS;
}
