#include "fs/fd.h"
#include "fs/vfs.h"
#include "mm/memory.h"
#include "utils/utils.h"
#include "drivers/vga/vga.h"
#include "drivers/video/bga/gfx_console.h"
#include "drivers/keyboard/keyboardDriver.h"

extern void net_close_socket(int pcb_id);
extern void net_close_tcp_socket(int pcb_id); 

KFile system_open_files[MAX_OPEN_FILES] = {0};

void fd_init(void) {
    for(int i = 0; i < MAX_OPEN_FILES; i++) {
        system_open_files[i].in_use = 0;
        system_open_files[i].refcount = 0;
    }
}

void fd_inherit(Task *child, Task *parent) {
    for(int i = 0; i < MAX_FDS; i++) {
        child->fd_table[i] = parent->fd_table[i];
        int sys_fd = parent->fd_table[i];
        if(sys_fd != -1) {
            system_open_files[sys_fd].refcount++;
            if (system_open_files[sys_fd].type == FILE_TYPE_PIPE) {
                if (system_open_files[sys_fd].mode == O_RDONLY)
                    system_open_files[sys_fd].pipe->readers++;
                else
                    system_open_files[sys_fd].pipe->writers++;
            }
        }
    }
}

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
        if (flags & O_CREAT) { vfs_write(abs_path, (uint8_t*)"", 0); size = 0; } 
        else { return -1; }
    }

    if (flags & O_TRUNC) { size = 0; vfs_write(abs_path, (uint8_t*)"", 0); }

    system_open_files[sys_fd].in_use = 1;
    system_open_files[sys_fd].refcount = 1;
    system_open_files[sys_fd].type = FILE_TYPE_NORMAL;
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

int sys_pipe_impl(int *fds) {
    int fd0 = -1, fd1 = -1;
    for (int i = 3; i < MAX_FDS; i++) {
        if (current_task->fd_table[i] == -1) {
            if (fd0 == -1) fd0 = i; else { fd1 = i; break; }
        }
    }
    if (fd1 == -1) return -1;

    int sys_fd0 = -1, sys_fd1 = -1;
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (!system_open_files[i].in_use) {
            if (sys_fd0 == -1) sys_fd0 = i; else { sys_fd1 = i; break; }
        }
    }
    if (sys_fd1 == -1) return -1;

    PipeData *p = kmalloc(sizeof(PipeData));
    fast_memset(p, 0, sizeof(PipeData) / 4);
    p->readers = 1; p->writers = 1;

    system_open_files[sys_fd0].in_use = 1;
    system_open_files[sys_fd0].refcount = 1;
    system_open_files[sys_fd0].type = FILE_TYPE_PIPE;
    system_open_files[sys_fd0].mode = O_RDONLY;
    system_open_files[sys_fd0].pipe = p;

    system_open_files[sys_fd1].in_use = 1;
    system_open_files[sys_fd1].refcount = 1;
    system_open_files[sys_fd1].type = FILE_TYPE_PIPE;
    system_open_files[sys_fd1].mode = O_WRONLY;
    system_open_files[sys_fd1].pipe = p;

    current_task->fd_table[fd0] = sys_fd0;
    current_task->fd_table[fd1] = sys_fd1;

    fds[0] = fd0; fds[1] = fd1;
    return 0;
}

int sys_close(int fd) {
    if (fd < 0 || fd >= MAX_FDS) return -1;
    int sys_fd = current_task->fd_table[fd];
    if (sys_fd == -1) return -1;

    current_task->fd_table[fd] = -1;
    KFile *kf = &system_open_files[sys_fd];

    if (kf->type == FILE_TYPE_PIPE) {
        if (kf->mode == O_RDONLY) kf->pipe->readers--;
        else kf->pipe->writers--;
        
        if (kf->pipe->readers <= 0 && kf->pipe->writers <= 0) {
            kfree(kf->pipe);
            kf->pipe = NULL;
        }
    }

    kf->refcount--;
    
    if (kf->refcount <= 0) {
        if (kf->type == FILE_TYPE_SOCKET) { 
            net_close_socket(kf->socket_id);
            if (kf->buffer) kfree(kf->buffer);
        } else if (kf->type == FILE_TYPE_SOCKET_TCP) { 
            net_close_tcp_socket(kf->socket_id);
            if (kf->buffer) kfree(kf->buffer);
        } else if (kf->type == FILE_TYPE_NORMAL) {
            if (kf->is_dirty) vfs_write(kf->path, kf->buffer, kf->size); 
            if (kf->buffer) kfree(kf->buffer);
        }
        kf->in_use = 0;
    }
    return 0;
}

int sys_read_fd(int fd, uint8_t *buf, uint32_t count) {
    if (fd < 0 || fd >= MAX_FDS) return -1;
    int sys_fd = current_task->fd_table[fd];
    
    if (sys_fd == -1) {
        if (fd == 0) { buf[0] = (uint8_t)getch(); return 1; }
        return -1;
    }

    KFile *kf = &system_open_files[sys_fd];
    
    if (kf->type == FILE_TYPE_PIPE) {
        PipeData *p = kf->pipe;
        
        
        if (count == 0) {
            if (p->count > 0) return p->count; 
            if (p->writers <= 0) return -1;    
            return 0;                          
        }

        uint32_t read_bytes = 0;
        while(read_bytes < count) {
            if (p->count > 0) {
                buf[read_bytes++] = p->buffer[p->head];
                p->head = (p->head + 1) % 4096;
                p->count--;
            } else {
                if (p->writers <= 0) break; 
                yield(); 
            }
        }
        return read_bytes;
    }

    if (kf->offset >= kf->size) return 0;
    uint32_t to_read = count;
    if (kf->offset + count > kf->size) to_read = kf->size - kf->offset;
    fast_memcpy(buf, kf->buffer + kf->offset, to_read);
    kf->offset += to_read;
    return to_read;
}

extern void vesa_render_buffer(void);
extern void gfx_start_batch(void);
extern void gfx_end_batch(void);

int sys_write_fd(int fd, uint8_t *buf, uint32_t count) {
    if (fd < 0 || fd >= MAX_FDS) return -1;
    int sys_fd = current_task->fd_table[fd];
    
    if (sys_fd == -1) {
        if (fd == 1 || fd == 2) {
            gfx_start_batch();
            uint32_t i = 0;
            while (i < count) {
                unsigned int code = 0;
                const char* next = utf8_to_unicode((char*)&buf[i], &code);
                int bytes_read = next - (char*)&buf[i];
                if (bytes_read <= 0 || i + bytes_read > count) gfx_putc(buf[i++]);
                else { gfx_putc(code); i += bytes_read; }
            }
            gfx_end_batch();
            
            return count;
        }
        return -1;
    }

    KFile *kf = &system_open_files[sys_fd];
    
    if (kf->type == FILE_TYPE_PIPE) {
        PipeData *p = kf->pipe;
        uint32_t written = 0;
        while (written < count) {
            if (p->readers <= 0) return -1; 
            if (p->count < 4096) {
                p->buffer[p->tail] = buf[written++];
                p->tail = (p->tail + 1) % 4096;
                p->count++;
            } else {
                yield(); 
            }
        }
        return written;
    }

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
    if (fd < 0 || fd >= MAX_FDS) return -1;
    int sys_fd = current_task->fd_table[fd];
    if (sys_fd == -1) return -1;

    KFile *kf = &system_open_files[sys_fd];
    if (kf->type == FILE_TYPE_PIPE) return -1; 
    
    if (whence == 0) kf->offset = offset; 
    else if (whence == 1) kf->offset += offset; 
    else if (whence == 2) kf->offset = kf->size + offset; 
    return kf->offset;
}