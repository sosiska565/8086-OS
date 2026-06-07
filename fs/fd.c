#include "fs/fd.h"
#include "fs/vfs.h"
#include "mm/memory.h"
#include "task/task.h"
#include "utils/utils.h"
#include "drivers/vga/vga.h"

#define MAX_OPEN_FILES 256
#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR   2
#define O_CREAT  0x0200
#define O_TRUNC  0x0400

typedef struct {
    int in_use;
    char path[256];
    uint8_t *buffer;
    uint32_t size;
    uint32_t offset;
    uint32_t capacity;
    int mode;
    int is_dirty;
} KFile;

KFile system_open_files[MAX_OPEN_FILES] = {0};

int sys_open(char *path, int flags) {
    char abs_path[256];
    get_absolute_path(current_task->cwd, path, abs_path);

    int free_fd = -1;
    for (int i = 3; i < MAX_FDS; i++) { 
        if (current_task->fd_table[i] == -1) { free_fd = i; break; }
    }
    if (free_fd == -1) return -1; 

    int sys_fd = -1;
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (!system_open_files[i].in_use) { sys_fd = i; break; }
    }
    if (sys_fd == -1) return -1; 

    int size = vfs_get_size(abs_path);
    
    if (size <= 0) {
        if (flags & O_CREAT) {
            vfs_write(abs_path, (uint8_t*)"", 0); 
            size = 0;
        } else {
            return -1; 
        }
    }

    if (flags & O_TRUNC) {
        size = 0;
        vfs_write(abs_path, (uint8_t*)"", 0);
    }

    system_open_files[sys_fd].in_use = 1;
    strcpy(system_open_files[sys_fd].path, abs_path);
    system_open_files[sys_fd].size = size;
    system_open_files[sys_fd].offset = 0;
    system_open_files[sys_fd].mode = flags;
    system_open_files[sys_fd].is_dirty = 0;
    
    system_open_files[sys_fd].capacity = size + 4096;
    system_open_files[sys_fd].buffer = kmalloc(system_open_files[sys_fd].capacity); 
    if (size > 0) vfs_read(abs_path, system_open_files[sys_fd].buffer);

    current_task->fd_table[free_fd] = sys_fd;
    return free_fd;
}

int sys_close(int fd) {
    if (fd < 3 || fd >= MAX_FDS) return -1;
    int sys_fd = current_task->fd_table[fd];
    if (sys_fd == -1) return -1;

    KFile *kf = &system_open_files[sys_fd];
    if (kf->is_dirty) {
        vfs_write(kf->path, kf->buffer, kf->size); 
    }
    
    kfree(kf->buffer);
    kf->in_use = 0;
    current_task->fd_table[fd] = -1;
    return 0;
}

int sys_read_fd(int fd, uint8_t *buf, uint32_t count) {
    if (fd < 3 || fd >= MAX_FDS) return -1;
    int sys_fd = current_task->fd_table[fd];
    if (sys_fd == -1) return -1;

    KFile *kf = &system_open_files[sys_fd];
    if (kf->offset >= kf->size) return 0;

    uint32_t to_read = count;
    if (kf->offset + count > kf->size) to_read = kf->size - kf->offset;

    fast_memcpy(buf, kf->buffer + kf->offset, to_read);
    kf->offset += to_read;
    return to_read;
}

int sys_write_fd(int fd, uint8_t *buf, uint32_t count) {
    if (fd < 3 || fd >= MAX_FDS) return -1;
    int sys_fd = current_task->fd_table[fd];
    if (sys_fd == -1) return -1;

    KFile *kf = &system_open_files[sys_fd];
    
    uint32_t needed = kf->offset + count;
    if (needed > kf->capacity) {
        uint32_t new_cap = needed + 4096;
        uint8_t *new_buf = kmalloc(new_cap);
        fast_memcpy(new_buf, kf->buffer, kf->size);
        kfree(kf->buffer);
        kf->buffer = new_buf;
        kf->capacity = new_cap;
    }

    fast_memcpy(kf->buffer + kf->offset, buf, count);
    kf->offset += count;
    if (kf->offset > kf->size) kf->size = kf->offset;
    kf->is_dirty = 1;
    return count;
}

int sys_lseek(int fd, int offset, int whence) {
    if (fd < 3 || fd >= MAX_FDS) return -1;
    int sys_fd = current_task->fd_table[fd];
    if (sys_fd == -1) return -1;

    KFile *kf = &system_open_files[sys_fd];
    if (whence == 0) kf->offset = offset; 
    else if (whence == 1) kf->offset += offset; 
    else if (whence == 2) kf->offset = kf->size + offset; 
    return kf->offset;
}