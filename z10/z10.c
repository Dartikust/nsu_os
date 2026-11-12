#define _XOPEN_SOURCE 600
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s command [args ...]\n", argv[0]);
        return 1;
    }

    pid_t child = fork();
    if (child == -1) {
        perror("Error: fork");
        return 1;
    }
    if (child == 0) {
        execvp(argv[1], argv + 1);
        fprintf(stderr, "Error: cannot execute '%s': %s\n", argv[1], strerror(errno));
        _exit(127);
    }

    int status;
    if (waitpid(child, &status, 0) == -1) {
        perror("Error: waitpid");
        return 1;
    }

    if (WIFEXITED(status)) {
        int code = WEXITSTATUS(status);
        printf("Exit code: %d\n", code);
        if (code != 0)
            fprintf(stderr, "Error: command '%s' exited with code %d.\n", argv[1], code);
        return code;
    }
    if (WIFSIGNALED(status)) {
        int signal = WTERMSIG(status);
        fprintf(stderr, "Error: command '%s' terminated by signal %d.\n", argv[1], signal);
        return 128 + signal;
    }
    return 1;
}
