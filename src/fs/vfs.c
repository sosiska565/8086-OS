#include "fs/vfs.h"
#include "fs/fat/fat32.h"
#include "drivers/video/bga/gfx_console.h"
#include "drivers/video/graphics.h"
#include "drivers/keyboard/keyboardDriver.h"
#include "multitask/task.h"
#include "utils/utils.h"
#include "drivers/vga/vga.h"

void vfs_init(void) {
    printf("[VFS] Virtual File System initialized.\n");
    printf("[VFS] Mounted DevFS at /dev/\n");
}

int vfs_read(char* path, uint8_t* buffer) {    
    if (strcmp(path, "/dev/stdin") == 0) {
        buffer[0] = (uint8_t)getch();
        buffer[1] = '\0';
        return 1;
    }
    
    if (strcmp(path, "/dev/urandom") == 0 || strcmp(path, "/dev/random") == 0) {
        buffer[0] = (uint8_t)(random() % 256);
        return 1;
    }
    
    return fat32_read_file(path, buffer);
}

int vfs_write(char* path, uint8_t* buffer, uint32_t size) {
    if (strcmp(path, "/dev/stdout") == 0 || strcmp(path, "/dev/stderr") == 0) {
        for(uint32_t i = 0; i < size; i++) {
            if (current_task && current_task->window) {
                window_putc(current_task->window, buffer[i]);
            } else {
                gfx_putc(buffer[i]); 
            }
        }
        return size;
    }
    
    if (strcmp(path, "/dev/null") == 0) {
        return size;
    }
    return fat32_write_file(path, buffer, size);
}

int vfs_get_size(char* path) {
    if (strcmp(path, "/dev/stdin") == 0) return 1;
    if (strcmp(path, "/dev/stdout") == 0) return 0;
    if (strcmp(path, "/dev/stderr") == 0) return 0;
    if (strcmp(path, "/dev/random") == 0) return 1;
    if (strcmp(path, "/dev/null") == 0) return 0;
    
    return fat32_get_file_size(path);
}