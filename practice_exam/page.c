// page.c
// Usage: ./page <num_frames> <p1> <p2> ...
// Outputs:
// FIFO: <faults>
// OPT:  <faults>

#include <stdio.h>
#include <stdlib.h>

static int contains(const int *frames, int fcount, int page) {
    for (int i = 0; i < fcount; i++) if (frames[i] == page) return 1;
    return 0;
}

static int fifo_faults(const int *refs, int rcount, int num_frames) {
    int *frames = (int *)malloc((size_t)num_frames * sizeof(int));
    if (!frames) { perror("malloc"); exit(1); }

    for (int i = 0; i < num_frames; i++) frames[i] = -1;

    int faults = 0;
    int hand = 0;

    for (int i = 0; i < rcount; i++) {
        int p = refs[i];
        if (contains(frames, num_frames, p)) continue;

        faults++;
        frames[hand] = p;
        hand = (hand + 1) % num_frames;
    }

    free(frames);
    return faults;
}

static int opt_faults(const int *refs, int rcount, int num_frames) {
    int *frames = (int *)malloc((size_t)num_frames * sizeof(int));
    if (!frames) { perror("malloc"); exit(1); }

    for (int i = 0; i < num_frames; i++) frames[i] = -1;

    int filled = 0;
    int faults = 0;

    for (int i = 0; i < rcount; i++) {
        int p = refs[i];
        if (contains(frames, num_frames, p)) continue;

        faults++;

        // free slot?
        if (filled < num_frames) {
            frames[filled++] = p;
            continue;
        }

        // choose victim: page with farthest next use (or never used again)
        int victim = -1;
        int farthest = -1;

        for (int f = 0; f < num_frames; f++) {
            int fp = frames[f];

            int next = -1;
            for (int j = i + 1; j < rcount; j++) {
                if (refs[j] == fp) { next = j; break; }
            }

            if (next == -1) { // never used again => best victim immediately
                victim = f;
                farthest = 1e9;
                break;
            }

            if (next > farthest) {
                farthest = next;
                victim = f;
            }
        }

        frames[victim] = p;
    }

    free(frames);
    return faults;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <num_frames> <p1> <p2> ...\n", argv[0]);
        return 1;
    }

    int num_frames = atoi(argv[1]);
    if (num_frames <= 0) {
        fprintf(stderr, "num_frames must be > 0\n");
        return 1;
    }

    int rcount = argc - 2;
    int *refs = (int *)malloc((size_t)rcount * sizeof(int));
    if (!refs) { perror("malloc"); return 1; }

    for (int i = 0; i < rcount; i++) refs[i] = atoi(argv[i + 2]);

    int f = fifo_faults(refs, rcount, num_frames);
    int o = opt_faults(refs, rcount, num_frames);

    printf("FIFO: %d\n", f);
    printf("OPT: %d\n", o);

    free(refs);
    return 0;
}
