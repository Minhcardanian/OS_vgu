#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define BUFFER_SIZE 4096

int main() {
    char src[256], dest[256];
    int fd_src, fd_dest;
    ssize_t bytesRead, bytesWritten;
    char buffer[BUFFER_SIZE];

    // Prompt for file names
    printf("Enter source file: ");
    scanf("%255s", src);
    printf("Enter destination file: ");
    scanf("%255s", dest);

    // Open source file
    fd_src = open(src, O_RDONLY);
    if (fd_src < 0) {
        fprintf(stderr, "Error opening source file '%s': %s\n", src, strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Open destination file (create if needed, truncate if exists)
    fd_dest = open(dest, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd_dest < 0) {
        fprintf(stderr, "Error opening destination file '%s': %s\n", dest, strerror(errno));        close(fd_src);
        exit(EXIT_FAILURE);
    }

    // Copy loop
    while ((bytesRead = read(fd_src, buffer, BUFFER_SIZE)) > 0) {
        bytesWritten = write(fd_dest, buffer, bytesRead);
        if (bytesWritten != bytesRead) {
            fprintf(stderr, "Write error: %s\n", strerror(errno));
            close(fd_src);
            close(fd_dest);
            exit(EXIT_FAILURE);
        }
    }
}
