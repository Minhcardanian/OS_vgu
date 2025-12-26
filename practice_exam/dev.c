// dev.c
// Usage: ./dev n1 n2 n3 ...
// Outputs integer-part mean absolute deviation and population standard deviation.

#include <pthread.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static int *arr = NULL;
static int n = 0;

static int g_mean = 0;
static int g_mad = 0;
static int g_std = 0;

static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cv = PTHREAD_COND_INITIALIZER;
static int mean_ready = 0;

static void *mad_worker(void *arg) {
    (void)arg;

    // compute integer-part mean
    long long sum = 0;
    for (int i = 0; i < n; i++) sum += arr[i];
    int mean = (int)(sum / n);

    // publish mean
    pthread_mutex_lock(&mtx);
    g_mean = mean;
    mean_ready = 1;
    pthread_cond_broadcast(&cv);
    pthread_mutex_unlock(&mtx);

    // compute mean absolute deviation (using integer mean)
    long long abs_sum = 0;
    for (int i = 0; i < n; i++) {
        long long d = (long long)arr[i] - (long long)mean;
        if (d < 0) d = -d;
        abs_sum += d;
    }
    g_mad = (int)(abs_sum / n); // integer part
    return NULL;
}

static void *std_worker(void *arg) {
    (void)arg;

    // wait for mean
    pthread_mutex_lock(&mtx);
    while (!mean_ready) pthread_cond_wait(&cv, &mtx);
    int mean = g_mean;
    pthread_mutex_unlock(&mtx);

    // population variance: (1/n) * sum (xi-mean)^2
    long double sq_sum = 0.0L;
    for (int i = 0; i < n; i++) {
        long double d = (long double)arr[i] - (long double)mean;
        sq_sum += d * d;
    }
    long double var = sq_sum / (long double)n;
    long double sd = sqrtl(var);

    g_std = (int)sd; // integer part
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

    pthread_t t_mad, t_std;
    if (pthread_create(&t_mad, NULL, mad_worker, NULL) != 0) {
        perror("pthread_create");
        free(arr);
        return 1;
    }
    if (pthread_create(&t_std, NULL, std_worker, NULL) != 0) {
        perror("pthread_create");
        pthread_join(t_mad, NULL);
        free(arr);
        return 1;
    }

    pthread_join(t_mad, NULL);
    pthread_join(t_std, NULL);

    printf("The mean absolute deviation value is %d\n", g_mad);
    printf("The standard deviation value is %d\n", g_std);

    free(arr);
    return 0;
}
