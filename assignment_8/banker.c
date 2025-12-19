/*
 * Banker's Algorithm (Interactive) — banker.c
 *
 * Build:
 *   gcc -O2 -Wall -Wextra -std=c11 banker.c -o banker
 *
 * Run:
 *   ./banker 10 5 7 8
 *   ./banker 10 5 7 8 -f max.txt
 *
 * max.txt: NUMBER_OF_CUSTOMERS lines, each with NUMBER_OF_RESOURCES integers
 * separators can be spaces and/or commas.
 *
 * Commands:
 *   RQ <cid> r0 r1 ... r(m-1)
 *   RL <cid> r0 r1 ... r(m-1)
 *   *
 *   exit | quit
 */

/* Enable POSIX prototypes (e.g., strtok_r) under -std=c11 */
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>   /* strcasecmp */
#include <ctype.h>
#include <errno.h>

#define NUMBER_OF_CUSTOMERS 5
#define MAX_LINE 4096

static int NUMBER_OF_RESOURCES = 0;

/* Banker state */
static int *available = NULL;     /* [m] */
static int **maximum = NULL;      /* [n][m] */
static int **allocation = NULL;   /* [n][m] */
static int **need = NULL;         /* [n][m] */

static void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

static void *xcalloc(size_t n, size_t sz) {
    void *p = calloc(n, sz);
    if (!p) die("calloc");
    return p;
}

static int **alloc_matrix(int rows, int cols) {
    int **mat = (int **)xcalloc((size_t)rows, sizeof(int *));
    for (int i = 0; i < rows; i++) {
        mat[i] = (int *)xcalloc((size_t)cols, sizeof(int));
    }
    return mat;
}

static void free_matrix(int **mat, int rows) {
    if (!mat) return;
    for (int i = 0; i < rows; i++) free(mat[i]);
    free(mat);
}

static void trim_newline(char *s) {
    if (!s) return;
    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r')) {
        s[len - 1] = '\0';
        len--;
    }
}

/* Parse a line with ints separated by commas/spaces/tabs.
   Returns how many ints parsed into out (up to cap). */
static int parse_ints_from_line(const char *line, int *out, int cap) {
    int count = 0;
    const char *p = line;

    while (*p && count < cap) {
        while (*p && (isspace((unsigned char)*p) || *p == ',')) p++;
        if (!*p) break;

        errno = 0;
        char *end = NULL;
        long v = strtol(p, &end, 10);
        if (p == end) break;
        if (errno != 0) return -1;
        if (v < 0 || v > 1000000000L) return -1;

        out[count++] = (int)v;
        p = end;
    }

    return count;
}

static void recompute_need(void) {
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
        for (int j = 0; j < NUMBER_OF_RESOURCES; j++) {
            need[i][j] = maximum[i][j] - allocation[i][j];
        }
    }
}

static void print_vector(const char *name, const int *v) {
    printf("%s: [", name);
    for (int j = 0; j < NUMBER_OF_RESOURCES; j++) {
        printf("%d", v[j]);
        if (j + 1 < NUMBER_OF_RESOURCES) printf(" ");
    }
    printf("]\n");
}

static void print_matrix(const char *name, int **mat) {
    printf("%s:\n", name);
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
        printf("  C%d: [", i);
        for (int j = 0; j < NUMBER_OF_RESOURCES; j++) {
            printf("%d", mat[i][j]);
            if (j + 1 < NUMBER_OF_RESOURCES) printf(" ");
        }
        printf("]\n");
    }
}

static void print_state(void) {
    printf("\n=== BANKER STATE ===\n");
    print_vector("Available ", available);
    print_matrix("Maximum   ", maximum);
    print_matrix("Allocation", allocation);
    print_matrix("Need      ", need);
    printf("====================\n\n");
}

/* Safety algorithm (Banker's):
   work = available
   finish[i] = false
   If exists i with finish[i]==false and need[i] <= work:
      work += allocation[i]; finish[i]=true
   safe if all finish true */
static int is_safe_state(int *safe_seq /* [n] or NULL */) {
    int *work = (int *)xcalloc((size_t)NUMBER_OF_RESOURCES, sizeof(int));
    int *finish = (int *)xcalloc((size_t)NUMBER_OF_CUSTOMERS, sizeof(int));

    for (int j = 0; j < NUMBER_OF_RESOURCES; j++) work[j] = available[j];

    int count = 0;
    int progressed = 1;

    while (count < NUMBER_OF_CUSTOMERS && progressed) {
        progressed = 0;
        for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
            if (finish[i]) continue;

            int can_finish = 1;
            for (int j = 0; j < NUMBER_OF_RESOURCES; j++) {
                if (need[i][j] > work[j]) {
                    can_finish = 0;
                    break;
                }
            }

            if (can_finish) {
                for (int j = 0; j < NUMBER_OF_RESOURCES; j++) {
                    work[j] += allocation[i][j];
                }
                finish[i] = 1;
                if (safe_seq) safe_seq[count] = i;
                count++;
                progressed = 1;
            }
        }
    }

    int safe = (count == NUMBER_OF_CUSTOMERS);

    free(work);
    free(finish);
    return safe;
}

/* request_resources() returns 0 if granted, -1 if denied */
static int request_resources(int customer_num, const int request[]) {
    if (customer_num < 0 || customer_num >= NUMBER_OF_CUSTOMERS) {
        fprintf(stderr, "ERROR: customer id out of range.\n");
        return -1;
    }

    for (int j = 0; j < NUMBER_OF_RESOURCES; j++) {
        if (request[j] < 0) {
            fprintf(stderr, "ERROR: request cannot be negative.\n");
            return -1;
        }
        if (request[j] > need[customer_num][j]) {
            fprintf(stderr, "DENIED: request exceeds customer need.\n");
            return -1;
        }
        if (request[j] > available[j]) {
            fprintf(stderr, "DENIED: request exceeds available resources.\n");
            return -1;
        }
    }

    /* Tentatively allocate */
    for (int j = 0; j < NUMBER_OF_RESOURCES; j++) {
        available[j] -= request[j];
        allocation[customer_num][j] += request[j];
        need[customer_num][j] -= request[j];
    }

    int *seq = (int *)xcalloc((size_t)NUMBER_OF_CUSTOMERS, sizeof(int));
    int safe = is_safe_state(seq);

    if (safe) {
        printf("GRANTED: system remains SAFE. Safe sequence: ");
        for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
            printf("C%d", seq[i]);
            if (i + 1 < NUMBER_OF_CUSTOMERS) printf(" -> ");
        }
        printf("\n");
        free(seq);
        return 0;
    }

    /* Rollback */
    for (int j = 0; j < NUMBER_OF_RESOURCES; j++) {
        available[j] += request[j];
        allocation[customer_num][j] -= request[j];
        need[customer_num][j] += request[j];
    }
    free(seq);

    printf("DENIED: request would leave system UNSAFE.\n");
    return -1;
}

static void release_resources(int customer_num, const int release[]) {
    if (customer_num < 0 || customer_num >= NUMBER_OF_CUSTOMERS) {
        fprintf(stderr, "ERROR: customer id out of range.\n");
        return;
    }

    for (int j = 0; j < NUMBER_OF_RESOURCES; j++) {
        if (release[j] < 0) {
            fprintf(stderr, "ERROR: release cannot be negative.\n");
            return;
        }
        if (release[j] > allocation[customer_num][j]) {
            fprintf(stderr, "ERROR: release exceeds allocated amount.\n");
            return;
        }
    }

    for (int j = 0; j < NUMBER_OF_RESOURCES; j++) {
        allocation[customer_num][j] -= release[j];
        available[j] += release[j];
        need[customer_num][j] += release[j];
    }

    printf("RELEASED: resources returned to bank.\n");
}

/* Reads maximum matrix from file_path. Expects exactly NUMBER_OF_CUSTOMERS lines. */
static void load_maximum_from_file(const char *file_path) {
    FILE *fp = fopen(file_path, "r");
    if (!fp) die("fopen(max file)");

    char line[MAX_LINE];
    int row = 0;

    while (row < NUMBER_OF_CUSTOMERS && fgets(line, sizeof(line), fp)) {
        trim_newline(line);

        int all_space = 1;
        for (const char *q = line; *q; q++) {
            if (!isspace((unsigned char)*q) && *q != ',') { all_space = 0; break; }
        }
        if (all_space) continue;
        if (line[0] == '#') continue;

        int *tmp = (int *)xcalloc((size_t)NUMBER_OF_RESOURCES, sizeof(int));
        int got = parse_ints_from_line(line, tmp, NUMBER_OF_RESOURCES);
        if (got != NUMBER_OF_RESOURCES) {
            fprintf(stderr,
                    "ERROR: line %d in %s must contain exactly %d integers.\n",
                    row + 1, file_path, NUMBER_OF_RESOURCES);
            free(tmp);
            fclose(fp);
            exit(EXIT_FAILURE);
        }

        for (int j = 0; j < NUMBER_OF_RESOURCES; j++) {
            maximum[row][j] = tmp[j];
        }
        free(tmp);
        row++;
    }

    fclose(fp);

    if (row != NUMBER_OF_CUSTOMERS) {
        fprintf(stderr,
                "ERROR: %s must contain %d customer lines (found %d).\n",
                file_path, NUMBER_OF_CUSTOMERS, row);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
        for (int j = 0; j < NUMBER_OF_RESOURCES; j++) {
            if (maximum[i][j] < 0) {
                fprintf(stderr, "ERROR: maximum must be non-negative.\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

static void usage(const char *prog) {
    fprintf(stderr,
            "Usage:\n"
            "  %s r0 r1 ... r(m-1) [-f maxfile]\n\n"
            "Examples:\n"
            "  %s 10 5 7 8\n"
            "  %s 10 5 7 8 -f max.txt\n\n"
            "Commands:\n"
            "  RQ <cid> <r0..r(m-1)>   request\n"
            "  RL <cid> <r0..r(m-1)>   release\n"
            "  *                       print state\n"
            "  exit|quit               end\n",
            prog, prog, prog);
}

static int is_number_str(const char *s) {
    if (!s || !*s) return 0;
    for (const char *p = s; *p; p++) {
        if (!isdigit((unsigned char)*p)) return 0;
    }
    return 1;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    const char *maxfile = "max.txt";

    /* Parse args: resources until optional -f */
    int resource_count = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-f") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "ERROR: -f requires a filename.\n");
                return EXIT_FAILURE;
            }
            maxfile = argv[i + 1];
            break;
        }
        if (!is_number_str(argv[i])) {
            fprintf(stderr, "ERROR: resource values must be non-negative integers.\n");
            usage(argv[0]);
            return EXIT_FAILURE;
        }
        resource_count++;
    }

    if (resource_count <= 0) {
        fprintf(stderr, "ERROR: you must provide at least 1 resource type.\n");
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    NUMBER_OF_RESOURCES = resource_count;

    /* Allocate structures */
    available = (int *)xcalloc((size_t)NUMBER_OF_RESOURCES, sizeof(int));
    maximum = alloc_matrix(NUMBER_OF_CUSTOMERS, NUMBER_OF_RESOURCES);
    allocation = alloc_matrix(NUMBER_OF_CUSTOMERS, NUMBER_OF_RESOURCES);
    need = alloc_matrix(NUMBER_OF_CUSTOMERS, NUMBER_OF_RESOURCES);

    /* Init available from argv */
    for (int j = 0; j < NUMBER_OF_RESOURCES; j++) {
        long v = strtol(argv[1 + j], NULL, 10);
        if (v < 0 || v > 1000000000L) {
            fprintf(stderr, "ERROR: invalid resource amount.\n");
            return EXIT_FAILURE;
        }
        available[j] = (int)v;
    }

    /* Load maximum and init need (allocation starts 0) */
    load_maximum_from_file(maxfile);
    recompute_need();

    printf("Banker's Algorithm initialized.\n");
    printf("Customers: %d, Resource types: %d\n", NUMBER_OF_CUSTOMERS, NUMBER_OF_RESOURCES);
    printf("Max file: %s\n", maxfile);
    printf("Enter commands (RQ/RL/*/exit).\n");
    print_state();

    char cmdline[MAX_LINE];
    while (1) {
        printf("> ");
        if (!fgets(cmdline, sizeof(cmdline), stdin)) {
            printf("\nEOF received. Exiting.\n");
            break;
        }
        trim_newline(cmdline);

        char *p = cmdline;
        while (*p && isspace((unsigned char)*p)) p++;
        if (*p == '\0') continue;

        if (strcmp(p, "*") == 0) {
            print_state();
            continue;
        }

        if (strcasecmp(p, "exit") == 0 || strcasecmp(p, "quit") == 0) {
            break;
        }

        /* Tokenize (re-entrant) */
        char *tokens[64];
        int nt = 0;
        char *save = NULL;
        char *tok = strtok_r(p, " \t", &save);
        while (tok && nt < (int)(sizeof(tokens) / sizeof(tokens[0]))) {
            tokens[nt++] = tok;
            tok = strtok_r(NULL, " \t", &save);
        }

        if (nt == 0) continue;

        if (strcasecmp(tokens[0], "RQ") == 0 || strcasecmp(tokens[0], "RL") == 0) {
            if (nt != 2 + NUMBER_OF_RESOURCES) {
                fprintf(stderr,
                        "ERROR: expected format: %s <cid> %d integers\n",
                        tokens[0], NUMBER_OF_RESOURCES);
                continue;
            }

            errno = 0;
            char *end = NULL;
            long cidL = strtol(tokens[1], &end, 10);
            if (errno != 0 || end == tokens[1] || *end != '\0') {
                fprintf(stderr, "ERROR: invalid customer id.\n");
                continue;
            }
            int cid = (int)cidL;

            int *vec = (int *)xcalloc((size_t)NUMBER_OF_RESOURCES, sizeof(int));
            int ok = 1;
            for (int j = 0; j < NUMBER_OF_RESOURCES; j++) {
                errno = 0;
                end = NULL;
                long v = strtol(tokens[2 + j], &end, 10);
                if (errno != 0 || end == tokens[2 + j] || *end != '\0' || v < 0 || v > 1000000000L) {
                    ok = 0;
                    break;
                }
                vec[j] = (int)v;
            }

            if (!ok) {
                fprintf(stderr, "ERROR: invalid resource vector.\n");
                free(vec);
                continue;
            }

            if (strcasecmp(tokens[0], "RQ") == 0) {
                (void)request_resources(cid, vec);
            } else {
                release_resources(cid, vec);
            }

            free(vec);
            continue;
        }

        fprintf(stderr, "ERROR: unknown command. Use RQ, RL, *, exit.\n");
    }

    /* Cleanup */
    free(available);
    free_matrix(maximum, NUMBER_OF_CUSTOMERS);
    free_matrix(allocation, NUMBER_OF_CUSTOMERS);
    free_matrix(need, NUMBER_OF_CUSTOMERS);

    printf("Done.\n");
    return EXIT_SUCCESS;
}
