enum
{

    SYS_FD_STDIN = 0,
    SYS_FD_STDOUT = 1,
    SYS_FD_STDERR = 2

};

unsigned int sys_read(unsigned int fd, void *buffer, unsigned int count);
unsigned int sys_write(unsigned int fd, void *buffer, unsigned int count);
unsigned int sys_open(char *path);
void sys_close(unsigned int fd);
void sys_seek(unsigned int fd, unsigned int offset);
