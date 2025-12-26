
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

static int *arr = NULL;
static int n = 0;

static int g_mean = 0;
static int g_median = 0;

static int cmp_int(const void *a, const void *b) {
    int x = *(const int *)a;
    int y = *(const int *)b;
    return (x > y) - (x < y);
}

static void *mean_worker(void *arg) {
    (void)arg;
    long long sum = 0;
    for (int i = 0; i < n; i++) sum += arr[i];
    g_mean = (int)(sum / n); // integer part
    return NULL;
}

static void *median_worker(void *arg) {
    (void)arg;
    int *tmp = (int *)malloc((size_t)n * sizeof(int));
    if (!tmp) {
        perror("malloc");
        pthread_exit(NULL);
    }
    for (int i = 0; i < n; i++) tmp[i] = arr[i];

    qsort(tmp, (size_t)n, sizeof(int), cmp_int);

    if (n % 2 == 1) {
        g_median = tmp[n / 2];
    } else {
        int a = tmp[n / 2 - 1];
        int b = tmp[n / 2];
        g_median = (a + b) / 2; 
    }

    free(tmp);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s n1 n2 n3 ...\n", argv[0]);
        return 1;
    }

    n = argc - 1;
    arr = (int *)malloc((size_t)n * sizeof(int));
    if (!arr) {
        perror("malloc");
        return 1;
    }

    for (int i = 0; i < n; i++) {
        arr[i] = atoi(argv[i + 1]);
    }

    pthread_t t1, t2;
    if (pthread_create(&t1, NULL, mean_worker, NULL) != 0) {
        perror("pthread_create");
        free(arr);
        return 1;
    }
    if (pthread_create(&t2, NULL, median_worker, NULL) != 0) {
        perror("pthread_create");
        pthread_join(t1, NULL);
        free(arr);
        return 1;
    }

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    printf("The average value is %d\n", g_mean);
    printf("The median value is %d\n", g_median);

    free(arr);
    return 0;
}
