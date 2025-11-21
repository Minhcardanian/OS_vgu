#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s command [args...]\n", argv[0]);
        return 1;
    }

    struct timeval start, end;
    pid_t pid;
    int status;

    if (gettimeofday(&start, NULL) == -1) {
        perror("gettimeofday");
        return 1;
    }

    pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    } else if (pid == 0) {
        execvp(argv[1], &argv[1]);
        perror("execvp");
        _exit(1);
    }

    if (waitpid(pid, &status, 0) == -1) {
        perror("waitpid");
        return 1;
    }

    if (gettimeofday(&end, NULL) == -1) {
        perror("gettimeofday");
        return 1;
    }

    double elapsed = (end.tv_sec - start.tv_sec)
                   + (end.tv_usec - start.tv_usec) / 1000000.0;

    printf("Elapsed time: %.6f seconds\n", elapsed);

    if (WIFEXITED(status))
        return WEXITSTATUS(status);
    return 1;
}

