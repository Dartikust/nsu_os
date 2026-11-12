#define _XOPEN_SOURCE 600
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

extern char **environ;

int execvpe(const char *file, char *const argv[], char *const envp[])
{
    char **original = environ;
    environ = (char **)envp;

    execvp(file, argv);

    environ = original;
    return -1;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s command [args ...]\n", argv[0]);
        return 1;
    }

    char *environment[] = {
        "PATH=/usr/bin:/bin",
        "LAB11=hello",
        NULL
    };

    execvpe(argv[1], argv + 1, environment);
    fprintf(stderr, "Error: cannot execute '%s': %s\n", argv[1], strerror(errno));
    return 1;
}
