#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "sys.h"

enum
{

    SYS_READ = 0,
    SYS_WRITE = 1,
    SYS_OPEN = 2,
    SYS_CLOSE = 3,
    SYS_SEEK = 8

};

unsigned int sys_read(unsigned int fd, void *buffer, unsigned int count)
{

    int ret = syscall(SYS_READ, fd, buffer, count);

    if (ret < 0)
    {

        dprintf(SYS_STDERR, "Read syscall failed (%d)\n", ret);
        exit(EXIT_FAILURE);

    }

    return ret;

}

unsigned int sys_write(unsigned int fd, void *buffer, unsigned int count)
{

    int ret = syscall(SYS_WRITE, fd, buffer, count);

    if (ret < 0)
    {

        dprintf(SYS_STDERR, "Write syscall failed (%d)\n", ret);
        exit(EXIT_FAILURE);

    }

    return ret;

}

unsigned int sys_open(char *path)
{

    int ret = syscall(SYS_OPEN, path, 0);

    if (ret < 0)
    {

        dprintf(SYS_STDERR, "Open syscall failed (%d)\n", ret);
        exit(EXIT_FAILURE);

    }

    return ret;

}

void sys_close(unsigned int fd)
{

    int ret = syscall(SYS_CLOSE, fd);

    if (ret < 0)
    {

        dprintf(SYS_STDERR, "Close syscall failed (%d)\n", ret);
        exit(EXIT_FAILURE);

    }

}

void sys_seek(unsigned int fd, unsigned int offset)
{

    int ret = syscall(SYS_SEEK, fd, offset, 0);

    if (ret < 0)
    {

        dprintf(SYS_STDERR, "Seek syscall failed (%d)\n", ret);
        exit(EXIT_FAILURE);

    }

}

