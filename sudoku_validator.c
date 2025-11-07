#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#define N 9

typedef struct {
    const int (*grid)[N];   // pointer to 9x9 grid
    int start_row;          // for subgrid: top-left row
    int start_col;          // for subgrid: top-left col
    int result_index;       // index into results[]
} task_t;

static int results[11];     // [0]=rows, [1]=cols, [2..10]=9 subgrids

// ---- Utils ----
static int check_1_to_9(const int* arr, int len) {
    int seen[10] = {0};
    for (int i = 0; i < len; ++i) {
        int v = arr[i];
        if (v < 1 || v > 9) return 0;        // out of range
        if (seen[v]) return 0;               // duplicate
        seen[v] = 1;
    }
    return 1;
}

// ---- Workers ----
static void* check_all_rows(void* arg) {
    task_t* t = (task_t*)arg;
    for (int r = 0; r < N; ++r) {
        int buf[N];
        for (int c = 0; c < N; ++c) buf[c] = t->grid[r][c];
        if (!check_1_to_9(buf, N)) {
            results[t->result_index] = 0;
            return NULL;
        }
    }
    results[t->result_index] = 1;
    return NULL;
}

static void* check_all_cols(void* arg) {
    task_t* t = (task_t*)arg;
    for (int c = 0; c < N; ++c) {
        int buf[N];
        for (int r = 0; r < N; ++r) buf[r] = t->grid[r][c];
        if (!check_1_to_9(buf, N)) {
            results[t->result_index] = 0;
            return NULL;
        }
    }
    results[t->result_index] = 1;
    return NULL;
}

static void* check_subgrid(void* arg) {
    task_t* t = (task_t*)arg;
    int buf[N];
    int k = 0;
    for (int r = t->start_row; r < t->start_row + 3; ++r)
        for (int c = t->start_col; c < t->start_col + 3; ++c)
            buf[k++] = t->grid[r][c];
    results[t->result_index] = check_1_to_9(buf, N);
    return NULL;
}

// ---- I/O ----
static int load_grid_from_stdin(int grid[N][N]) {
    int count = 0;
    for (int r = 0; r < N; ++r)
        for (int c = 0; c < N; ++c) {
            if (scanf("%d", &grid[r][c]) != 1) return 0;
            count++;
        }
    return count == N * N;
}

static void load_default_grid(int grid[N][N]) {
    // Valid Sudoku from the textbook figure
    int g[N][N] = {
        {6,2,4,5,3,9,1,8,7},
        {5,1,9,7,2,8,6,3,4},
        {8,3,7,6,1,4,2,9,5},
        {1,4,3,8,6,5,7,2,9},
        {9,5,8,2,4,7,3,6,1},
        {7,6,2,3,9,1,4,5,8},
        {3,7,1,9,5,6,8,4,2},
        {4,9,6,1,8,2,5,7,3},
        {2,8,5,4,7,3,9,1,6}
    };
    memcpy(grid, g, sizeof g);
}

int main(void) {
    int grid[N][N];
    if (!load_grid_from_stdin(grid)) {
        fprintf(stderr, "[info] No/insufficient input detected — using built-in valid grid.\n");
        load_default_grid(grid);
    }

    // zero results
    for (int i = 0; i < 11; ++i) results[i] = 0;

    pthread_t th[11];
    task_t tasks[11];
    int tcount = 0;

    // rows checker -> results[0]
    tasks[tcount] = (task_t){ .grid = (const int (*)[N])grid, .start_row = 0, .start_col = 0, .result_index = 0 };
    pthread_create(&th[tcount], NULL, check_all_rows, &tasks[tcount]);
    tcount++;

    // cols checker -> results[1]
    tasks[tcount] = (task_t){ .grid = (const int (*)[N])grid, .start_row = 0, .start_col = 0, .result_index = 1 };
    pthread_create(&th[tcount], NULL, check_all_cols, &tasks[tcount]);
    tcount++;

    // 9 subgrid checkers -> results[2..10]
    int idx = 2;
    for (int sr = 0; sr < N; sr += 3) {
        for (int sc = 0; sc < N; sc += 3) {
            tasks[tcount] = (task_t){ .grid = (const int (*)[N])grid,
                                      .start_row = sr, .start_col = sc,
                                      .result_index = idx++ };
            pthread_create(&th[tcount], NULL, check_subgrid, &tasks[tcount]);
            tcount++;
        }
    }

    // join all
    for (int i = 0; i < tcount; ++i) pthread_join(th[i], NULL);

    // aggregate
    int valid = 1;
    for (int i = 0; i < 11; ++i) if (results[i] != 1) { valid = 0; break; }

    // report
    printf("Rows valid     : %s\n", results[0] ? "YES" : "NO");
    printf("Columns valid  : %s\n", results[1] ? "YES" : "NO");
    for (int i = 0; i < 9; ++i)
        printf("Subgrid %d valid: %s\n", i+1, results[2+i] ? "YES" : "NO");
    printf("\nSudoku solution is %s\n", valid ? "VALID" : "INVALID");
    return valid ? 0 : 1;
}
