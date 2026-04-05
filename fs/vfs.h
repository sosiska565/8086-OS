#ifndef VFS_H
#define VFS_H

#include <stdint.h>

#define VFS_ATTR_FILE 0
#define VFS_ATTR_DIR  1
#define VFS_ATTR_DEV  2

typedef struct {
    char name[64];
    uint32_t size;
    uint8_t type;
} vfs_dirent_t;

void vfs_init(void);

int vfs_read(char* path, uint8_t* buffer);
int vfs_write(char* path, uint8_t* buffer, uint32_t size);
int vfs_get_size(char* path);

int vfs_readdir(char* path, int index, vfs_dirent_t* out_dirent);

int vfs_get_attr(char* path, uint8_t* out_type);
int vfs_mkdir(char* path);
int vfs_delete(char* path); 

#endif