#define _XOPEN_SOURCE 600
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/mman.h>

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

static int build_table(const char *data, size_t size,
                       struct line **table, size_t *count)
{
    size_t start = 0;
    for (size_t i = 0; i < size; ++i) {
        if (data[i] == '\n') {
            if (add_line(table, count, (off_t)start, i + 1 - start) == -1)
                return -1;
            start = i + 1;
        }
    }
    if (start < size)
        return add_line(table, count, (off_t)start, size - start);
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
    struct stat info;
    char *data = NULL;
    size_t size = 0;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s text-file\n", argv[0]);
        return 1;
    }
    fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror(argv[1]);
        return 1;
    }
    if (fstat(fd, &info) == -1) {
        perror("fstat");
        status = 1;
        goto cleanup;
    }
    size = (size_t)info.st_size;
    if (size > 0) {
        data = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (data == MAP_FAILED) {
            perror("mmap");
            close(fd);
            return 1;
        }
    }
    if (build_table(data, size, &table, &count) == -1) {
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
            if (size > 0)
                fwrite(data, 1, size, stdout);
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
        struct line selected = table[number - 1];
        fwrite(data + selected.offset, 1, selected.length, stdout);
    }

cleanup:
    free(table);
    if (size > 0 && data != NULL)
        munmap(data, size);
    close(fd);
    return status;
}
