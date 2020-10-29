#include "kernel/types.h"
#include "user/user.h"
#define LACK_WRONG "lack parameter\n"

int main(int argc, char *argv[])
{
    if(argc < 2){
        write(2, LACK_WRONG, strlen(LACK_WRONG));
        exit(1);
    }

    int time = atoi(argv[1]);
    sleep(time);
    exit(0);
}
