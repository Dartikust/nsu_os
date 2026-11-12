#define _XOPEN_SOURCE 600
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s file\n", argv[0]);
        return 1;
    }

    pid_t child = fork();
    if (child == -1) {
        perror("fork");
        return 1;
    }
    if (child == 0) {
        execlp("cat", "cat", argv[1], (char *)NULL);
        perror("cat");
        _exit(127);
    }

    printf("Parent: child created.\n");
    printf("Parent: done.\n");
    return 0;
}
