#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

char* fmtname(char *path)
{
    static char buf[DIRSIZ+1];
    char* p;

    //Find first character after last slash
    for(p=path+strlen(path); p >= path && *p != '/'; p--)
        ;
    p++;

    //Return blank-padded name
    if(strlen(p) >= DIRSIZ)
        return p;
    memmove(buf, p, strlen(p));
    memset(buf+strlen(p), 0, DIRSIZ-strlen(p));
    return buf;
}

void find(char *path, char *file)
{
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;

    if((fd = open(path, 0)) < 0){
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }

    if(fstat(fd, &st) < 0){
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }

    // fprintf(2, "%s\n", path);
    // fprintf(2, "%s\n", fmtname(path));

    switch(st.type){
        case T_FILE:
            // printf("%s\n", fmtname(path));
            // printf("%s\n", file);
            if(strcmp(fmtname(path), file) == 0) {
                fprintf(1, "%s\n", path);
            }
            break;
        case T_DIR:
            if(strlen(path) + 1 + DIRSIZ + 1 > sizeof(buf)) {
                fprintf(2, "find: path too long\n");
                break;
            }
            strcpy(buf, path);
            p = buf + strlen(buf);
            *p++ = '/';
            while(read(fd, &de, sizeof(de)) == sizeof(de)){
                if(de.inum == 0)
                    continue;
                if(strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
                    continue;
                memmove(p, de.name, DIRSIZ);
                p[DIRSIZ] = 0;
                // if(stat(buf, &st) < 0){
                //     fprintf(2, "find: cannot stat %s\n", buf);
                //     continue;
                // }
                find(buf, file);
            }
            break;
        default:
            break;
    }
    close(fd);
}

int main(int argc, char *argv[])
{
    if(argc < 2){
        fprintf(2, "find: expect at lest 1 argument but 0");
        exit(1);
    }

    if(argc < 3){
        find(".", argv[1]);
        exit(0);
    }

    find(argv[1], argv[2]);
    exit(0);
}