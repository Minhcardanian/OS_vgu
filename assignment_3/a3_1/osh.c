// Minimal UNIX shell supporting: foreground/background, !! history,
// single input/output redirection (< or >), and a single pipe (cmd1 | cmd2).
// No mixing pipe with redirection in the same line to keep within the spec.

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>

#define MAX_ARGS 64
#define MAX_LINE 1024

static char *last_cmd = NULL;

static void trim(char *s){
    size_t n = strlen(s);
    while (n && (s[n-1]=='\n' || s[n-1]=='\r' || s[n-1]==' ' || s[n-1]=='\t')) s[--n]=0;
}

static int tokenize(char *line, char *argv[], int *is_bg){
    int argc = 0;
    *is_bg = 0;
    char *tok = strtok(line, " \t");
    while (tok && argc < MAX_ARGS-1){
        argv[argc++] = tok;
        tok = strtok(NULL, " \t");
    }
    if (argc && strcmp(argv[argc-1],"&")==0){
        *is_bg = 1; argv[--argc] = NULL;
    }
    argv[argc] = NULL;
    return argc;
}

static void exec_simple(char *argv[], char *infile, char *outfile){
    if (infile){
        int fd = open(infile, O_RDONLY);
        if (fd < 0){ perror("open <"); _exit(1); }
        if (dup2(fd, STDIN_FILENO) < 0){ perror("dup2 <"); _exit(1); }
        close(fd);
    }
    if (outfile){
        int fd = open(outfile, O_WRONLY|O_CREAT|O_TRUNC, 0666);
        if (fd < 0){ perror("open >"); _exit(1); }
        if (dup2(fd, STDOUT_FILENO) < 0){ perror("dup2 >"); _exit(1); }
        close(fd);
    }
    execvp(argv[0], argv);
    perror("execvp");
    _exit(127);
}

static int find_token(char *argv[], const char *sym){
    for (int i=0; argv[i]; ++i) if (strcmp(argv[i], sym)==0) return i;
    return -1;
}

int main(void){
    char line[MAX_LINE];

    while (1){
        printf("osh> ");
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin)) break;
        trim(line);
        if (line[0]==0) continue;

        // history !!
        if (strcmp(line, "!!")==0){
            if (!last_cmd){ puts("No commands in history."); continue; }
            strncpy(line, last_cmd, sizeof(line)-1);
            line[sizeof(line)-1]=0;
            printf("%s\n", line);
        } else {
            free(last_cmd);
            last_cmd = strdup(line);
        }

        // built-in exit
        if (!strcmp(line,"exit")) break;

        // tokenize to detect special symbols
        char work[MAX_LINE];
        strncpy(work, line, sizeof(work)); work[sizeof(work)-1]=0;
        char *argv[MAX_ARGS]; int is_bg=0;
        tokenize(work, argv, &is_bg);
        if (!argv[0]) continue;

        // single pipe?
        int ppos = find_token(argv, "|");
        if (ppos >= 0){
            // For simplicity, do not allow < or > in piped commands
            argv[ppos] = NULL;
            char *leftv[MAX_ARGS]; char *rightv[MAX_ARGS];
            int li=0, ri=0;
            for (int i=0; argv[i]; ++i) leftv[li++]=argv[i];
            leftv[li]=NULL;
            for (int i=ppos+1; argv[i]; ++i) rightv[ri++]=argv[i];
            rightv[ri]=NULL;

            int fds[2];
            if (pipe(fds)<0){ perror("pipe"); continue; }
            pid_t c1 = fork();
            if (c1==0){
                // left: stdout -> pipe
                close(fds[0]);
                if (dup2(fds[1], STDOUT_FILENO)<0){ perror("dup2 pipe left"); _exit(1); }
                close(fds[1]);
                execvp(leftv[0], leftv);
                perror("execvp left");
                _exit(127);
            }
            pid_t c2 = fork();
            if (c2==0){
                // right: stdin <- pipe
                close(fds[1]);
                if (dup2(fds[0], STDIN_FILENO)<0){ perror("dup2 pipe right"); _exit(1); }
                close(fds[0]);
                execvp(rightv[0], rightv);
                perror("execvp right");
                _exit(127);
            }
            close(fds[0]); close(fds[1]);
            // pipes always in foreground for this project
            waitpid(c1, NULL, 0);
            waitpid(c2, NULL, 0);
            continue;
        }

        // handle single redirection
        char *infile=NULL, *outfile=NULL;
        int lt = find_token(argv, "<");
        int gt = find_token(argv, ">");
        if (lt >= 0){
            if (!argv[lt+1]){ fprintf(stderr, "syntax: < file\n"); continue; }
            infile = argv[lt+1]; argv[lt]=NULL;
        }
        if (gt >= 0){
            if (!argv[gt+1]){ fprintf(stderr, "syntax: > file\n"); continue; }
            outfile = argv[gt+1]; argv[gt]=NULL;
        }

        pid_t pid = fork();
        if (pid==0){
            exec_simple(argv, infile, outfile);
        } else if (pid>0){
            if (!is_bg) waitpid(pid, NULL, 0);
        } else {
            perror("fork");
        }
    }

    free(last_cmd);
    return 0;
}

