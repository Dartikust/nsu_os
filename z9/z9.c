#define _XOPEN_SOURCE 600
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s file\n", argv[0]);
        return 1;
    }

    char path[PATH_MAX];
    struct stat info;
    if (realpath(argv[1], path) == NULL ||
        stat(path, &info) == -1 || access(path, R_OK) == -1) {
        perror(argv[1]);
        return 1;
    }
    if (!S_ISREG(info.st_mode)) {
        fprintf(stderr, "%s: expected a regular text file\n", argv[1]);
        return 1;
    }

    pid_t child = fork();   // 2 process 
    if (child == -1) {
        perror("fork");
        return 1;
    }
    if (child == 0) {       // 2nd working
        execlp("cat", "cat", path, (char *)NULL);
        perror("cat");
        _exit(127);
    }

    printf("Parent: child created.\n");
    fflush(stdout);

    int status;
    if (waitpid(child, &status, 0) == -1) { // waiting result
        perror("waitpid");
        return 1;
    }

    if (!WIFEXITED(status)) {
        fprintf(stderr, "cat did not exit normally.\n");
        return 1;
    }
    int code = WEXITSTATUS(status);
    if (code != 0) {
        fprintf(stderr, "cat failed (exit code %d).\n", code);
        return code;
    }

    printf("Parent: done.\n");
    return 0;
}
