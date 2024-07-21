#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

unsigned int syscall_read(unsigned int descriptor, void *buffer, unsigned int count)
{

    int ret = syscall(0, descriptor, buffer, count);

    if (ret < 0)
    {

        dprintf(2, "Read syscall failed (%d)\n", ret);
        exit(EXIT_FAILURE);

    }

    return ret;

}

unsigned int syscall_open(char *path)
{

    int ret = syscall(2, path, 0);

    if (ret < 0)
    {

        dprintf(2, "Read syscall failed (%d)\n", ret);
        exit(EXIT_FAILURE);

    }

    return ret;

}

void syscall_close(unsigned int descriptor)
{

    int ret = syscall(3, descriptor);

    if (ret < 0)
    {

        dprintf(2, "Close syscall failed (%d)\n", ret);
        exit(EXIT_FAILURE);

    }

}

void syscall_seek(unsigned int descriptor, unsigned int offset)
{

    int ret = syscall(8, descriptor, offset, 0);

    if (ret < 0)
    {

        dprintf(2, "Seek syscall failed (%d)\n", ret);
        exit(EXIT_FAILURE);

    }

}

