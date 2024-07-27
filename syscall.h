#define SYSCALL_STDIN 0
#define SYSCALL_STDOUT 1
#define SYSCALL_STDERR 2

unsigned int syscall_read(unsigned int descriptor, void *buffer, unsigned int count);
unsigned int syscall_open(char *path);
void syscall_close(unsigned int descriptor);
void syscall_seek(unsigned int descriptor, unsigned int offset);
