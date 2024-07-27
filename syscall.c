#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "syscall.h"

enum
{

    SYSCALL_READ = 0,
    SYSCALL_WRITE = 1,
    SYSCALL_OPEN = 2,
    SYSCALL_CLOSE = 3,
    SYSCALL_SEEK = 8

};

unsigned int syscall_read(unsigned int fd, void *buffer, unsigned int count)
{

    int ret = syscall(SYSCALL_READ, fd, buffer, count);

    if (ret < 0)
    {

        dprintf(SYSCALL_STDERR, "Read syscall failed (%d)\n", ret);
        exit(EXIT_FAILURE);

    }

    return ret;

}

unsigned int syscall_write(unsigned int fd, void *buffer, unsigned int count)
{

    int ret = syscall(SYSCALL_WRITE, fd, buffer, count);

    if (ret < 0)
    {

        dprintf(SYSCALL_STDERR, "Write syscall failed (%d)\n", ret);
        exit(EXIT_FAILURE);

    }

    return ret;

}

unsigned int syscall_open(char *path)
{

    int ret = syscall(SYSCALL_OPEN, path, 0);

    if (ret < 0)
    {

        dprintf(SYSCALL_STDERR, "Open syscall failed (%d)\n", ret);
        exit(EXIT_FAILURE);

    }

    return ret;

}

void syscall_close(unsigned int fd)
{

    int ret = syscall(SYSCALL_CLOSE, fd);

    if (ret < 0)
    {

        dprintf(SYSCALL_STDERR, "Close syscall failed (%d)\n", ret);
        exit(EXIT_FAILURE);

    }

}

void syscall_seek(unsigned int fd, unsigned int offset)
{

    int ret = syscall(SYSCALL_SEEK, fd, offset, 0);

    if (ret < 0)
    {

        dprintf(SYSCALL_STDERR, "Seek syscall failed (%d)\n", ret);
        exit(EXIT_FAILURE);

    }

}

