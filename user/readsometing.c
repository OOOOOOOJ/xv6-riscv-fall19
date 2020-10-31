#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

int main(int argc, char *argv[])
{
    char buf[512];
    int fd = open("./log", O_RDONLY);
    read(fd, buf, 512);
    printf("%s\n", buf);
    exit(0);
}