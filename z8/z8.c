#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>

int main(int argc, char **argv) {
    if (argc == 3 && (strcmp(argv[2], "1") == 0 || strcmp(argv[2], "2") == 0)) {
        int fd = open(argv[1], O_RDWR);
        if (fd == -1) {
            perror("open");
            return 1;
        }
        else {
            struct flock lock = {0};
            lock.l_type = F_WRLCK;
            lock.l_whence = SEEK_SET;
            lock.l_len = 0;
            
            mode_t original_mode = 0;  
            int status = 1;

            if (strcmp(argv[2], "2") == 0) {
                struct stat st;
                if (fstat(fd, &st) == -1) {
                    perror("fstat");
                    close(fd);
                    return 1;
                }
                original_mode = st.st_mode & 07777;
                mode_t mode = st.st_mode;
                mode |= S_ISGID;
                mode &= ~S_IXGRP;
                
                if (fchmod(fd, mode) == -1) {
                    perror("fchmod");
                    goto cleanup;
                }
            }

            if (fcntl(fd, F_SETLK, &lock) != -1) {
                printf("File locked! Exit nano to unlock.\n"); 

                if (setenv("Z8_FILE", argv[1], 1) == -1) {
                    perror("setenv");
                    goto cleanup;
                }

                int check = system("nano -- \"$Z8_FILE\"");
                if(check == -1) {
                    perror("system");
                    goto cleanup;
                }
                else if (check != 0) {
                    fprintf(stderr, "Nano command ended with error!");
                    goto cleanup;
                }

                status = 0;

                cleanup:
                    if (strcmp(argv[2], "2") == 0) {
                        if (fchmod(fd, original_mode) == -1) {
                            perror("restore permissions");
                            status = 1;
                        }
                    }
                    lock.l_type = F_UNLCK;
                    if (fcntl(fd, F_SETLK, &lock) == -1) {
                        perror("fcntl");
                        status = 1;
                    }
                    if (close(fd) == -1) {
                        perror("close");
                        status = 1;
                    }

                    return status;
            }

            else {
                perror("fcntl");
                goto cleanup;
            }
        }
    }
    else {
        printf("Write a name of txt file and the 1 if you want Advisory lock or 2 if you want Mandatory lock");
        return 1;
    }
}
