#include "kernel/types.h"
#include "user/user.h"

static void getPrime(int input)
{
    int divide, temp;
    int fd[2];
    int sig;

    if(read(input, &divide, sizeof(int)) <= 0) {
        exit(0);
    }

    printf("prime %d\n", divide);
    if(pipe(fd) < 0) {
        fprintf(2, "pipe error\n");
        exit(0);
    }

    if(fork() == 0) {
        close(fd[0]);
        while(read(input, &temp, sizeof(int)) > 0) {
            if(temp % divide != 0){
                write(fd[1], &temp, sizeof(int));
            }
        }
        close(fd[1]);
        close(input);
        exit(0);
    } else {
        close(fd[1]);
        wait(&sig);
        getPrime(fd[0]);
    }
}

void main(int argc, char *argv[])
{
    int fd[2];
    pipe(fd);
    int i;
    for(i=2; i<35; i++) {
        write(fd[1], &i, sizeof(int));
    }
    close(fd[1]);
    getPrime(fd[0]);
}