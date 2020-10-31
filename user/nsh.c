#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"
#include "kernel/param.h"

#define EXEC  1
#define REDIR 2
#define PIPE  3

#define MAXP 16

char whitespace[] = " \t\r\n\v";
char symbols[] = "<|>";

struct args {
    int arg_num;
    char arg[MAXARG][MAXP];
    int left_head;                  //左参数起点
    int token;                      //token位置
};

struct cmd {
    int type;
    int head;
    int end;
    struct cmd *lcmd;
    struct cmd *rcmd;
    int fd;
    int file;
    int mode;
};

void parsecmd(char *ps, struct args *arg);
int gettoken(struct args *arg);
int getcmd(char *buf, int nbuf);
void runcmd(struct args* arg, struct cmd *cmd, char **argv);
void redirect(int k, int pd[]);


void parsecmd(char *ps, struct args *arg)
{
    char *s = ps;
    int index = 0;

    while(*s) {
        // if(index > MAXARG) {
        //     fprintf(2, "too many args\n");
        //     exit(-1);
        // }
        while(*s && strchr(whitespace, *s)) s++;
        if(*s == 0) break;
        char *ep = s;
        while(*ep && !strchr(whitespace, *ep)) ep++;
        memset(arg->arg[index], 0, MAXP);
        memmove(arg->arg[index], s, (ep-s)*sizeof(char));
        s = ep;
        index++;
    }
    arg->arg_num = index;
    // fprintf(2, "args num %d\n", arg->arg_num);
    arg->left_head = 0;
}

int gettoken(struct args *arg)
{
    int index = arg->left_head;
    int token = 0;
    for(; index < arg->arg_num; index++) {
        if(strchr(symbols, arg->arg[index][0])) {
            token = arg->arg[index][0];
            break;
        }
    }
    arg->token = (index == arg->arg_num) ? -1 : index;
    return token;
}

int getcmd(char *buf, int nbuf)
{
    fprintf(2, "@ ");
    memset(buf, 0, nbuf);
    gets(buf, nbuf);
    if(buf[0] == 0)
        return -1;
    return 0;
}

void redirect(int k, int pd[])
{
    close(k);
    dup(pd[k]);
    close(pd[0]);
    close(pd[1]);
}

void runcmd(struct args* arg, struct cmd *cmd, char **argv)
{
    int i;
    int p[2];

    switch (cmd->type)
    {
    case EXEC:
        for(i=0; i<cmd->end-cmd->head; i++) {
            argv[i] = (char *) &arg->arg[cmd->head + i];
        }
        exec(argv[0], argv);
        break;
    
    case REDIR:
        close(cmd->fd);
        if(open(arg->arg[cmd->file], cmd->mode) < 0) {
            fprintf(2, "open file failed %s\n", arg->arg[cmd->file]);
            fprintf(2, "open file failed %s\n", arg->arg[cmd->file - 1]);
            exit(-1);
        }
        runcmd(arg, cmd->rcmd, argv);
        break;

    case PIPE:
        if(pipe(p) < 0) {
            fprintf(2, "pipe\n");
            exit(-1);
        }

        if(fork() == 0) {
            redirect(1, p);
            runcmd(arg, cmd->lcmd, argv);
        }

        if(fork() == 0) {
            redirect(0, p);
            runcmd(arg, cmd->rcmd, argv);
        }

        close(p[0]);
        close(p[1]);
        wait(0);
        wait(0);
        break;

    default:
        break;
    }
    exit(0);
}

int main(void)
{
    int fd;
    while((fd = open("console", O_RDWR)) >= 0) {
        if(fd >= 3) {
            close(fd);
            break;
        }
    }

    struct args arg;
    struct cmd cmds[MAXARG];

    char *argv[MAXARG];

    char buf[128];
    int token;

    int index;
    int pipe_index;
    
    while(getcmd(buf, sizeof(buf)) == 0)
    {
        memset(&arg, 0, sizeof(arg));
        parsecmd(buf, &arg);
        memset(cmds, 0, sizeof(cmds));

        token = gettoken(&arg);

        if(token == 0) {    //若就一条指令
            cmds[0].type = EXEC;
            cmds[0].head = arg.left_head;
            cmds[0].end = arg.arg_num;
            if(fork() == 0) {
                memset(argv, 0, sizeof(argv));
                runcmd(&arg, &cmds[0], argv);
            }
            wait(0);
            continue;
        }

        index = 1;
        pipe_index = 0;
        cmds[0].type = EXEC;
        cmds[0].head = arg.left_head;
        cmds[0].end = arg.token;
        arg.left_head = arg.token + 1;

        while(token != 0) {
            while(token != '|' && token != 0) {
                switch (token)
                {
                case '>':
                    cmds[index].type = REDIR;
                    cmds[index].file = arg.token + 1;
                    cmds[index].fd = 1;
                    cmds[index].mode = O_WRONLY|O_CREATE;
                    cmds[index].rcmd = &cmds[index-1];
                    index++;
                    break;
                
                case '<':
                    cmds[index].type = REDIR;
                    cmds[index].file = arg.token + 1;
                    cmds[index].fd = 0;
                    cmds[index].mode = O_RDONLY;
                    cmds[index].rcmd = &cmds[index-1];
                    index++;
                    break;

                default:
                    break;
                }
                token = gettoken(&arg);
                arg.left_head = arg.token + 1;
            }

            if(token == '|') {
                cmds[index].type = PIPE;
                cmds[index].lcmd = &cmds[index-1];
                pipe_index = index;
                index++;
                token = gettoken(&arg);
                
                cmds[index].type = EXEC;
                cmds[index].head = arg.left_head;
                if(token == 0) {
                    cmds[index].end = arg.arg_num;
                } else{
                    cmds[index].end = arg.token;
                }
                index++;
                arg.left_head = arg.token + 1;
            }

            if(token == 0) {
                if(pipe_index) {
                    cmds[pipe_index].rcmd = &cmds[index-1];
                    // fprintf(2, "%d\n", index-1);
                }
                break;
            }
        }

        if(pipe_index) {
            if(fork() == 0)
                runcmd(&arg, &cmds[pipe_index], argv);

            // fprintf(2, "%d\n", index);
            wait(0);
            continue;
        } else {
            if(fork() == 0) {
                runcmd(&arg, &cmds[index-1], argv);
            }
            wait(0);
            continue;
        }
    }
    exit(0);
}