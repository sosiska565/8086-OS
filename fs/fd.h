#ifndef FD_H
#define FD_H

#include <stdint.h>

int sys_open(char *path, int flags);
int sys_close(int fd);
int sys_read_fd(int fd, uint8_t *buf, uint32_t count);
int sys_write_fd(int fd, uint8_t *buf, uint32_t count);
int sys_lseek(int fd, int offset, int whence);

#endif