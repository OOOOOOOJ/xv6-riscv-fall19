#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

#define EXEC 1
#define REDIR 2
#define PIPE 3

char[] whitespace = " "

struct cmd {
    int type;
}

struct execcmd {
    int type;
    char *argv[MAXARG];
}

struct redircmd {
    int type;
    struct cmd *cmd;
    char *file;
    char *efile;
    int mode;
    int fd;
}

struct pipecmd {
    int type;
    struct cmd *left;
    struct cmd *right;
}

int gettoken(char **ps, char **es, char **q, char **eq)
{
    char *s;
    int ret;
    
    s = *ps;
    while(s < es && strchr())
}