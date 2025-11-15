#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TASKS     128
#define NAME_LEN      32
#define TIME_QUANTUM  10

typedef struct {
    char name[NAME_LEN];
    int priority;
    int burst;

    /* runtime fields */
    int remaining;
    int first_start;   /* -1 until first scheduled */
    int completion;
} Task;

/* -------------------- Input handling -------------------- */

static int load_tasks(const char *filename, Task tasks[], int max_tasks) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("fopen");
        return -1;
    }

    char line[256];
    int n = 0;
    while (fgets(line, sizeof(line), fp) && n < max_tasks) {
        /* Expected format:  T1, 4, 20   (commas or spaces) */
        char name[NAME_LEN];
        int prio, burst;

        line[strcspn(line, "\r\n")] = '\0';   /* strip newline */

        /* comma-separated */
        if (sscanf(line, " %31[^,],%d,%d", name, &prio, &burst) != 3) {
            /* fallback: space-separated */
            if (sscanf(line, " %31s %d %d", name, &prio, &burst) != 3) {
                fprintf(stderr, "Skipping malformed line: '%s'\n", line);
                continue;
            }
        }

        strncpy(tasks[n].name, name, NAME_LEN - 1);
        tasks[n].name[NAME_LEN - 1] = '\0';
        tasks[n].priority   = prio;
        tasks[n].burst      = burst;
        tasks[n].remaining  = burst;
        tasks[n].first_start = -1;
        tasks[n].completion  = 0;
        n++;
    }

    fclose(fp);
    return n;
}

static void reset_runtime(Task *dst, const Task *src, int n) {
    for (int i = 0; i < n; i++) {
        dst[i] = src[i];
        dst[i].remaining   = dst[i].burst;
        dst[i].first_start = -1;
        dst[i].completion  = 0;
    }
}

/* -------------------- Output helpers -------------------- */

static void print_metrics(const Task tasks[], int n) {
    double total_wait = 0.0, total_turn = 0.0, total_resp = 0.0;

    printf("\nTask  Burst  Start  Complete  Waiting  Turnaround  Response\n");
    printf("-------------------------------------------------------------------\n");

    for (int i = 0; i < n; i++) {
        int turnaround = tasks[i].completion;         /* arrival = 0 for all */
        int waiting    = turnaround - tasks[i].burst; /* no I/O, single CPU */
        int response   = (tasks[i].first_start < 0) ? 0 : tasks[i].first_start;

        total_wait += waiting;
        total_turn += turnaround;
        total_resp += response;

        printf("%-4s  %5d  %5d  %8d  %7d  %10d  %8d\n",
               tasks[i].name,
               tasks[i].burst,
               response,
               tasks[i].completion,
               waiting,
               turnaround,
               response);
    }

    printf("\nAverage waiting time   = %.2f\n", total_wait / n);
    printf("Average turnaround time= %.2f\n", total_turn / n);
    printf("Average response time  = %.2f\n", total_resp / n);
}

/* Append a summary of this run to output.txt */
static void log_summary_to_file(const char *algo_name,
                                const char *input_file,
                                const Task tasks[],
                                int n)
{
    FILE *fp = fopen("output.txt", "a");
    if (!fp) {
        perror("fopen output.txt");
        return;
    }

    double total_wait = 0.0, total_turn = 0.0, total_resp = 0.0;

    for (int i = 0; i < n; i++) {
        int turnaround = tasks[i].completion;
        int waiting    = turnaround - tasks[i].burst;
        int response   = (tasks[i].first_start < 0) ? 0 : tasks[i].first_start;

        total_wait += waiting;
        total_turn += turnaround;
        total_resp += response;
    }

    double avg_wait = total_wait / n;
    double avg_turn = total_turn / n;
    double avg_resp = total_resp / n;

    fprintf(fp, "==============================\n");
    fprintf(fp, "Algorithm : %s\n", algo_name);
    fprintf(fp, "Input file: %s\n", input_file);
    fprintf(fp, "Number of tasks: %d\n\n", n);

    fprintf(fp, "Task  Burst  Start  Complete  Waiting  Turnaround  Response\n");
    fprintf(fp, "-------------------------------------------------------------------\n");

    for (int i = 0; i < n; i++) {
        int turnaround = tasks[i].completion;
        int waiting    = turnaround - tasks[i].burst;
        int response   = (tasks[i].first_start < 0) ? 0 : tasks[i].first_start;

        fprintf(fp, "%-4s  %5d  %5d  %8d  %7d  %10d  %8d\n",
                tasks[i].name,
                tasks[i].burst,
                response,
                tasks[i].completion,
                waiting,
                turnaround,
                response);
    }

    fprintf(fp, "\nAverage waiting time   = %.2f\n", avg_wait);
    fprintf(fp, "Average turnaround time= %.2f\n", avg_turn);
    fprintf(fp, "Average response time  = %.2f\n\n", avg_resp);

    fclose(fp);
}

/* -------------------- FCFS -------------------- */

static void run_fcfs(const Task *orig, int n, const char *input_file) {
    Task t[MAX_TASKS];
    reset_runtime(t, orig, n);

    int time = 0;
    printf("\n==== First-Come, First-Served (FCFS) ====\n");

    for (int i = 0; i < n; i++) {
        if (t[i].first_start == -1)
            t[i].first_start = time;

        printf("[time %3d] Running %s for %d ms\n",
               time, t[i].name, t[i].burst);

        time += t[i].burst;
        t[i].remaining  = 0;
        t[i].completion = time;
    }

    print_metrics(t, n);
    log_summary_to_file("FCFS", input_file, t, n);
}

/* -------------------- SJF (non-preemptive) -------------------- */

static void run_sjf(const Task *orig, int n, const char *input_file) {
    Task t[MAX_TASKS];
    reset_runtime(t, orig, n);

    Task sorted[MAX_TASKS];
    int used[MAX_TASKS] = {0};

    /* selection sort by burst time (ascending) */
    for (int i = 0; i < n; i++) {
        int best = -1;
        for (int j = 0; j < n; j++) {
            if (used[j]) continue;
            if (best == -1 || t[j].burst < t[best].burst)
                best = j;
        }
        sorted[i] = t[best];
        used[best] = 1;
    }

    int time = 0;
    printf("\n==== Shortest-Job-First (SJF, non-preemptive) ====\n");

    for (int i = 0; i < n; i++) {
        if (sorted[i].first_start == -1)
            sorted[i].first_start = time;

        printf("[time %3d] Running %s for %d ms\n",
               time, sorted[i].name, sorted[i].burst);

        time += sorted[i].burst;
        sorted[i].remaining  = 0;
        sorted[i].completion = time;
    }

    print_metrics(sorted, n);
    log_summary_to_file("SJF", input_file, sorted, n);
}

/* -------------------- Priority (non-preemptive) -------------------- */
/* Higher numeric value = higher priority */

static void run_priority(const Task *orig, int n, const char *input_file) {
    Task t[MAX_TASKS];
    reset_runtime(t, orig, n);

    Task sorted[MAX_TASKS];
    int used[MAX_TASKS] = {0};

    /* sort by priority (descending) */
    for (int i = 0; i < n; i++) {
        int best = -1;
        for (int j = 0; j < n; j++) {
            if (used[j]) continue;
            if (best == -1 || t[j].priority > t[best].priority)
                best = j;
        }
        sorted[i] = t[best];
        used[best] = 1;
    }

    int time = 0;
    printf("\n==== Priority Scheduling (non-preemptive) ====\n");

    for (int i = 0; i < n; i++) {
        if (sorted[i].first_start == -1)
            sorted[i].first_start = time;

        printf("[time %3d] Running %s (prio=%d) for %d ms\n",
               time, sorted[i].name, sorted[i].priority, sorted[i].burst);

        time += sorted[i].burst;
        sorted[i].remaining  = 0;
        sorted[i].completion = time;
    }

    print_metrics(sorted, n);
    log_summary_to_file("PRIORITY", input_file, sorted, n);
}

/* -------------------- Round Robin -------------------- */

static void run_rr(const Task *orig, int n, const char *input_file) {
    Task t[MAX_TASKS];
    reset_runtime(t, orig, n);

    printf("\n==== Round Robin (quantum = %d ms) ====\n", TIME_QUANTUM);

    int queue[MAX_TASKS];
    int head = 0, tail = 0;

    for (int i = 0; i < n; i++)
        queue[tail++] = i;

    int time  = 0;
    int alive = n;

    while (alive > 0) {
        int idx = queue[head++];
        if (head == MAX_TASKS) head = 0;   /* wrap */

        Task *task = &t[idx];
        if (task->remaining <= 0) {
            /* already finished; skip */
            continue;
        }

        if (task->first_start == -1)
            task->first_start = time;

        int slice = (task->remaining < TIME_QUANTUM)
                    ? task->remaining : TIME_QUANTUM;

        printf("[time %3d] Running %s for %d ms (remaining before=%d)\n",
               time, task->name, slice, task->remaining);

        time += slice;
        task->remaining -= slice;

        if (task->remaining <= 0) {
            task->completion = time;
            alive--;
        } else {
            /* requeue */
            queue[tail++] = idx;
            if (tail == MAX_TASKS) tail = 0;
        }
    }

    print_metrics(t, n);
    log_summary_to_file("RR", input_file, t, n);
}

/* -------------------- Priority + Round Robin -------------------- */

static void run_priority_rr(const Task *orig, int n, const char *input_file) {
    Task t[MAX_TASKS];
    reset_runtime(t, orig, n);

    printf("\n==== Priority with Round Robin (quantum = %d ms) ====\n",
           TIME_QUANTUM);

    /* queues per priority 0..10 (we'll clamp input into this range) */
    int queues[11][MAX_TASKS];
    int q_head[11] = {0};
    int q_tail[11] = {0};

    int max_prio = 0;

    /* initial enqueue by priority */
    for (int i = 0; i < n; i++) {
        int p = t[i].priority;
        if (p < 0)  p = 0;
        if (p > 10) p = 10;
        if (p > max_prio) max_prio = p;

        queues[p][q_tail[p]++] = i;
    }

    int time  = 0;
    int alive = n;

    while (alive > 0) {
        /* find highest non-empty priority queue */
        int p;
        for (p = max_prio; p >= 0; p--) {
            if (q_head[p] != q_tail[p])
                break;
        }
        if (p < 0) {
            /* nothing runnable (should not happen) */
            break;
        }

        int idx = queues[p][q_head[p]++];
        if (q_head[p] == MAX_TASKS) q_head[p] = 0;

        Task *task = &t[idx];
        if (task->remaining <= 0) {
            /* stale entry, skip */
            continue;
        }

        if (task->first_start == -1)
            task->first_start = time;

        int slice = (task->remaining < TIME_QUANTUM)
                    ? task->remaining : TIME_QUANTUM;

        printf("[time %3d] Running %s (prio=%d) for %d ms (remaining before=%d)\n",
               time, task->name, task->priority, slice, task->remaining);

        time += slice;
        task->remaining -= slice;

        if (task->remaining <= 0) {
            task->completion = time;
            alive--;
        } else {
            /* requeue at same priority */
            queues[p][q_tail[p]++] = idx;
            if (q_tail[p] == MAX_TASKS) q_tail[p] = 0;
        }
    }

    print_metrics(t, n);
    log_summary_to_file("PRIORITY+RR", input_file, t, n);
}

/* -------------------- main() -------------------- */

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr,
                "Usage: %s schedule.txt [algo]\n"
                "  algo = fcfs | sjf | prio | rr | prio_rr | all (default: all)\n",
                argv[0]);
        return 1;
    }

    const char *filename = argv[1];
    const char *algo     = (argc >= 3) ? argv[2] : "all";

    Task tasks[MAX_TASKS];
    int n = load_tasks(filename, tasks, MAX_TASKS);
    if (n <= 0) {
        fprintf(stderr, "No tasks loaded.\n");
        return 1;
    }

    if (strcmp(algo, "fcfs") == 0) {
        run_fcfs(tasks, n, filename);
    } else if (strcmp(algo, "sjf") == 0) {
        run_sjf(tasks, n, filename);
    } else if (strcmp(algo, "prio") == 0) {
        run_priority(tasks, n, filename);
    } else if (strcmp(algo, "rr") == 0) {
        run_rr(tasks, n, filename);
    } else if (strcmp(algo, "prio_rr") == 0) {
        run_priority_rr(tasks, n, filename);
    } else { /* all */
        run_fcfs(tasks, n, filename);
        run_sjf(tasks, n, filename);
        run_priority(tasks, n, filename);
        run_rr(tasks, n, filename);
        run_priority_rr(tasks, n, filename);
    }

    return 0;
}
