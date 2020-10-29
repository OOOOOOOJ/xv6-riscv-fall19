#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

#define ARG_MAX_LENGTH 20

int main(int argc, char *argv[])
{
    if(argc < 2) {
        fprintf(2, "xargs: no command");
        exit(1);
    }

    // if(fork() == 0) {
    //     exec(argv[1], &argv[1]);
    //     fprintf(2, "exec %s failed\n", argv[1]);
    // }

    int sig;
    char *subargv[MAXARG];
    char argbuf[ARG_MAX_LENGTH];
    int arg_index = 0, argv_index = 0;
    int runflag = 0;

    for(; argv_index < argc-1; argv_index++) {
        subargv[argv_index] = malloc(ARG_MAX_LENGTH);
        memmove(subargv[argv_index], argv[argv_index+1], sizeof(argv[argv_index+1]));
    }

    // wait(&sig);

    while(read(0, argbuf + arg_index, sizeof(char)) > 0) {
            if(argbuf[arg_index] == '\n' || argbuf[arg_index] == ' ') {
                runflag = (argbuf[arg_index] == '\n') ? 1 : 0;
                argbuf[arg_index] = 0;
                if(subargv[argv_index] == 0)
                    subargv[argv_index] = malloc(ARG_MAX_LENGTH);
                else
                    memset(subargv[argv_index], 0, ARG_MAX_LENGTH);
                memmove(subargv[argv_index], argbuf, arg_index);
                argv_index++;
                arg_index = 0;
            }else if(arg_index == ARG_MAX_LENGTH) continue;
            else {
                arg_index++;
            }
            
            if(runflag) {
                while(subargv[argv_index] != 0){
                    memset(subargv[argv_index++], 0, ARG_MAX_LENGTH);
                }
                if(fork() == 0) {
                    exec(subargv[0], subargv);
                    fprintf(2, "exec %s failed\n", subargv[0]);
                }else {
                    wait(&sig);
                    runflag = 0;
                    arg_index = 0;
                    argv_index = argc - 1;
                }
            }
        }
    exit(0);
}