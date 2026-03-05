#ifndef VFS_H
#define VFS_H

#include <stdint.h>

void vfs_init(void);
int vfs_read(char* path, uint8_t* buffer);
int vfs_write(char* path, uint8_t* buffer, uint32_t size);
int vfs_get_size(char* path);

#endif