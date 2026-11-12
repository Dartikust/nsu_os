#define _XOPEN_SOURCE 600
#include <stdio.h>
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
        perror("fork");
        return 1;
    }
    if (child == 0) {
        execvp(argv[1], argv + 1);
        perror(argv[1]);
        _exit(127);
    }

    int status;
    if (waitpid(child, &status, 0) == -1) {
        perror("waitpid");
        return 1;
    }

    if (WIFEXITED(status)) {
        int code = WEXITSTATUS(status);
        printf("Exit code: %d\n", code);
        return code;
    }
    if (WIFSIGNALED(status)) {
        int signal = WTERMSIG(status);
        printf("Terminated by signal: %d\n", signal);
        return 128 + signal;
    }
    return 1;
}
