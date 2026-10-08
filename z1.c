#include <stdio.h>
#include <unistd.h>

int main (int argc, char *argv[]) {
    int opt;
    char path [1024];
    // getopt() возвращает символ найденной опции, а когда опции заканчиваются — -1.
    while ((opt = getopt(argc, argv, "ipds")) != -1) {
        switch (opt) {
            case 'i':
                printf("UID: %ld\n", (long)getuid()); // get user id (real)
                printf("EUID: %ld\n", (long)geteuid()); // get effective user id
                printf("GID: %ld\n", (long)getgid()); // get group id (real)
                printf("EGID: %ld\n", (long)getegid()); // get effective group id
                break;
            case 'p':
                printf("PID: %ld\n", (long)getpid()); // get process id
                printf("PPID: %ld\n", (long)getppid()); // get parent process id
                printf("PGRP: %ld\n", (long)getpgrp()); // get process group
                break;

            case 'd':
                if (getcwd(path, sizeof(path)) != NULL) {
                    printf("%s\n", path);
                } else {
                    perror("getcwd");
                }
                break;
            
            case 's':
                if (setpgid(0, 0) == 0) {
                    printf("Yes");
                } else { 
                    perror("setpgid");
                }
                break;

            default:
                
                break;
        }
    }
    return 0;
}