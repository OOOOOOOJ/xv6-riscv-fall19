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

    int sig;
    char *sub_argv[MAXARG];
    char arg_buf[ARG_MAX_LENGTH];
    int buf_index = 0, argv_index = 0;
    int runflag = 0;

    for(; argv_index < argc-1; argv_index++) {
        sub_argv[argv_index] = malloc(ARG_MAX_LENGTH);
        memmove(sub_argv[argv_index], argv[argv_index+1], sizeof(argv[argv_index+1]));
    }

    while(read(0, arg_buf + buf_index, sizeof(char)) > 0) {
            if(arg_buf[buf_index] == '\n' || arg_buf[buf_index] == ' ') {
                runflag = (arg_buf[buf_index] == '\n') ? 1 : 0;
                arg_buf[buf_index] = 0;
                if(sub_argv[argv_index] == 0)
                    sub_argv[argv_index] = malloc(ARG_MAX_LENGTH);
                else
                    memset(sub_argv[argv_index], 0, ARG_MAX_LENGTH);
                memmove(sub_argv[argv_index], arg_buf, buf_index);
                argv_index++;
                buf_index = 0;
            }else if(buf_index == ARG_MAX_LENGTH) continue;
            else {
                buf_index++;
            }
            
            if(runflag) {
                while(sub_argv[argv_index] != 0){
                    memset(sub_argv[argv_index++], 0, ARG_MAX_LENGTH);
                }
                if(fork() == 0) {
                    exec(sub_argv[0], sub_argv);
                    fprintf(2, "exec %s failed\n", sub_argv[0]);
                }else {
                    wait(&sig);
                    runflag = 0;
                    buf_index = 0;
                    argv_index = argc - 1;
                }
            }
        }
    exit(0);
}