#include "fs/vfs.h"
#include "fs/fat/fat32.h"
#include "drivers/video/bga/gfx_console.h"
#include "drivers/keyboard/keyboardDriver.h"
#include "task/task.h"
#include "utils/utils.h"
#include <stddef.h>
#include "drivers/vga/vga.h"
#include "mm/memory.h"
#include "drivers/timer/timer.h"

static const char* dev_files[] = {"stdin", "stdout", "stderr", "null", "random", "urandom", "zero"};
static const int num_dev_files = 7;

static const char* proc_files[] = {"meminfo", "cpuinfo", "version", "uptime"};
static const int num_proc_files = 4;

void vfs_init(void) { printf("[VFS] Virtual File System initialized.\n"); }

int vfs_read(char* path, uint8_t* buffer) {    
    
    if (strcmp(path, "/dev/stdin") == 0) { buffer[0] = (uint8_t)getch(); buffer[1] = '\0'; return 1; }
    if (strcmp(path, "/dev/urandom") == 0 || strcmp(path, "/dev/random") == 0) { buffer[0] = (uint8_t)(random() % 256); return 1; }
    if (strcmp(path, "/dev/zero") == 0) { buffer[0] = 0; return 1; }
    if (strcmp(path, "/dev/null") == 0 || strcmp(path, "/dev/stdout") == 0 || strcmp(path, "/dev/stderr") == 0) return 0;
    
    
    if (strncmp(path, "/proc/", 6) == 0) {
        char buf[512]; buf[0] = '\0';
        char temp[32];
        
        if (strcmp(path, "/proc/meminfo") == 0) {
            uint32_t used = get_used_memory() / 1024;
            uint32_t total = get_total_memory() / 1024;
            strcpy(buf, "MemTotal:  "); itoa(total, temp, 10); strcat(buf, temp); strcat(buf, " kB\n");
            strcat(buf, "MemFree:   "); itoa(total - used, temp, 10); strcat(buf, temp); strcat(buf, " kB\n");
        } 
        else if (strcmp(path, "/proc/cpuinfo") == 0) {
            char cpu[16]; get_cpu_vendor(cpu);
            strcpy(buf, "vendor_id\t: "); strcat(buf, cpu); strcat(buf, "\n");
        } 
        else if (strcmp(path, "/proc/version") == 0) {
            strcpy(buf, "8086-OS Linux-like Kernel v0.5\n");
        } 
        else if (strcmp(path, "/proc/uptime") == 0) {
            itoa(get_ticks() / 1000, temp, 10);
            strcpy(buf, temp); strcat(buf, " seconds\n");
        }
        
        int len = strlen(buf);
        strcpy((char*)buffer, buf);
        return len;
    }

    return fat32_read_file(path, buffer);
}

int vfs_write(char* path, uint8_t* buffer, uint32_t size) {
    if (strcmp(path, "/dev/stdout") == 0 || strcmp(path, "/dev/stderr") == 0) {
        for(uint32_t i = 0; i < size; i++) gfx_putc(buffer[i]); return size;
    }
    if (strcmp(path, "/dev/null") == 0) return size;
    if (strncmp(path, "/dev/", 5) == 0 || strncmp(path, "/proc/", 6) == 0) return -1;

    return fat32_write_file(path, buffer, size);
}

int vfs_get_size(char* path) {
    if (strncmp(path, "/dev/", 5) == 0) return 1; 
    if (strncmp(path, "/proc/", 6) == 0) return 512; 
    return fat32_get_file_size(path);
}

int vfs_get_attr(char* path, uint8_t* out_type) {
    if (strcmp(path, "/dev") == 0 || strcmp(path, "/dev/") == 0) { *out_type = VFS_ATTR_DIR; return 1; }
    if (strcmp(path, "/proc") == 0 || strcmp(path, "/proc/") == 0) { *out_type = VFS_ATTR_DIR; return 1; }
    
    for (int i = 0; i < num_dev_files; i++) {
        char p[64] = "/dev/"; strcat(p, (char*)dev_files[i]);
        if (strcmp(path, p) == 0) { *out_type = VFS_ATTR_DEV; return 1; }
    }
    for (int i = 0; i < num_proc_files; i++) {
        char p[64] = "/proc/"; strcat(p, (char*)proc_files[i]);
        if (strcmp(path, p) == 0) { *out_type = VFS_ATTR_FILE; return 1; }
    }
    
    uint32_t cluster = fat32_get_cluster_for_path(path, out_type, NULL);
    if (cluster == 0xFFFFFFFF) return 0;
    return 1;
}

int vfs_readdir(char* path, int index, vfs_dirent_t* out_dirent) {
    if (strcmp(path, "/dev") == 0 || strcmp(path, "/dev/") == 0) {
        if (index >= num_dev_files) return 0; 
        strcpy(out_dirent->name, (char*)dev_files[index]);
        out_dirent->size = 0; out_dirent->type = VFS_ATTR_DEV; return 1;
    }
    if (strcmp(path, "/proc") == 0 || strcmp(path, "/proc/") == 0) {
        if (index >= num_proc_files) return 0; 
        strcpy(out_dirent->name, (char*)proc_files[index]);
        out_dirent->size = 0; out_dirent->type = VFS_ATTR_FILE; return 1;
    }

    if (strcmp(path, "/") == 0) {
        if (fat32_readdir(path, index, out_dirent) == 1) return 1;
        int fat_count = 0; vfs_dirent_t temp;
        while (fat32_readdir(path, fat_count, &temp) == 1) fat_count++;

        if (index == fat_count) { strcpy(out_dirent->name, "dev"); out_dirent->type = VFS_ATTR_DIR; out_dirent->size = 0; return 1; }
        if (index == fat_count + 1) { strcpy(out_dirent->name, "proc"); out_dirent->type = VFS_ATTR_DIR; out_dirent->size = 0; return 1; }
        return 0;
    }

    return fat32_readdir(path, index, out_dirent);
}

int vfs_mkdir(char* path) {
    if (strncmp(path, "/dev", 4) == 0 || strncmp(path, "/proc", 5) == 0) return -1;
    return fat32_mkdir(path);
}

int vfs_delete(char* path) {
    if (strncmp(path, "/dev", 4) == 0 || strncmp(path, "/proc", 5) == 0) return -1; 
    return fat32_delete_file(path);
}