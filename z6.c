#define _XOPEN_SOURCE 600
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

struct line {
    off_t offset;
    size_t length;
};

static int add_line(struct line **table, size_t *count,
                    off_t offset, size_t length)
{
    struct line *grown = realloc(*table, (*count + 1) * sizeof(**table));
    if (grown == NULL) {
        perror("realloc");
        return -1;
    }
    *table = grown;
    grown[*count].offset = offset;
    grown[*count].length = length;
    ++*count;
    return 0;
}

static int build_table(int fd, struct line **table, size_t *count)
{
    char byte;
    off_t start = 0, position;
    ssize_t received;
    while ((received = read(fd, &byte, 1)) > 0) {
        if (byte == '\n') {
            position = lseek(fd, 0, SEEK_CUR);
            if (position == -1) {
                perror("lseek");
                return -1;
            }
            if (add_line(table, count, start, (size_t)(position - start)) == -1)
                return -1;
            start = position;
        }
    }
    if (received == -1) {
        perror("read");
        return -1;
    }
    position = lseek(fd, 0, SEEK_CUR);
    if (position == -1) {
        perror("lseek");
        return -1;
    }
    if (position > start)
        return add_line(table, count, start, (size_t)(position - start));
    return 0;
}

static int print_line(int fd, struct line line)
{
    char *text = malloc(line.length + 1);
    size_t total = 0;
    if (text == NULL) {
        perror("malloc");
        return -1;
    }
    if (lseek(fd, line.offset, SEEK_SET) == -1) {
        perror("lseek");
        free(text);
        return -1;
    }
    while (total < line.length) {
        ssize_t received = read(fd, text + total, line.length - total);
        if (received <= 0) {
            fprintf(stderr, "Cannot read the requested line\n");
            free(text);
            return -1;
        }
        total += (size_t)received;
    }
    text[line.length] = '\0';
    printf("%s", text);
    free(text);
    return 0;
}

static volatile sig_atomic_t expired = 0;

static void on_alarm(int signal_number)
{
    (void)signal_number;
    expired = 1;
    /* After timeout there will be no more input in this process. */
    close(STDIN_FILENO);
}

/* 1: number, 0: EOF/invalid input, 2: timeout, -1: read error. */
static int read_number(long *number)
{
    char input[64], byte;
    size_t used = 0;
    ssize_t received = 0;

    expired = 0;
    alarm(5);
    while (used < sizeof(input) - 1) {
        received = read(STDIN_FILENO, &byte, 1);
        if (received <= 0 || byte == '\n')
            break;
        input[used++] = byte;
    }
    alarm(0);
    if (expired)
        return 2;
    if (received == -1) {
        perror("read");
        return -1;
    }
    input[used] = '\0';
    return sscanf(input, "%ld", number) == 1;
}

int main(int argc, char **argv)
{
    int fd, status = 0;
    struct line *table = NULL;
    size_t count = 0;
    long number;
    struct sigaction action = {0};

    if (argc != 2) {
        fprintf(stderr, "Usage: %s text-file\n", argv[0]);
        return 1;
    }
    fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror(argv[1]);
        return 1;
    }
    if (build_table(fd, &table, &count) == -1) {
        status = 1;
        goto cleanup;
    }
    action.sa_handler = on_alarm;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    if (sigaction(SIGALRM, &action, NULL) == -1) {
        perror("sigaction");
        status = 1;
        goto cleanup;
    }
    for (;;) {
        printf("Line number (0 to quit), 5 seconds: ");
        fflush(stdout);
        int result = read_number(&number);
        if (result == 2) {
            puts("\nTime is up. Entire file:");
            for (size_t i = 0; i < count; ++i) {
                if (print_line(fd, table[i]) == -1) {
                    status = 1;
                    break;
                }
            }
            break;
        }
        if (result <= 0) {
            status = result == -1;
            break;
        }
        if (number == 0)
            break;
        if (number < 1 || (size_t)number > count) {
            puts("No such line.");
            continue;
        }
        if (print_line(fd, table[number - 1]) == -1) {
            status = 1;
            break;
        }
    }

cleanup:
    free(table);
    close(fd);
    return status;
}
