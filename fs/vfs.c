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
#include "drivers/file/ATA/ATA.h"

static const char* dev_files[] = {"stdin", "stdout", "stderr", "null", "random", "urandom", "zero"};
static const int num_dev_files = 7;


static const char* proc_files[] = {"meminfo", "cpuinfo", "version", "uptime", "klog", "disks"};
static const int num_proc_files = 6;

#define MAX_MOUNTS 16
mount_t mount_table[MAX_MOUNTS];

int vfs_mount(char* device, char* mount_point, char* fs_type) {
    int drive_id = -1;
    if (strncmp(device, "/dev/sd", 7) == 0 || strncmp(device, "/dev/hd", 7) == 0) {
        drive_id = device[7] - 'a';
    }
    
    if (drive_id < 0 || drive_id >= sys_drive_count) return -1; 

    if (strcmp(fs_type, "fat32") == 0) {
        if (fat32_mount(drive_id) != 0) return -2; 
    } else {
        return -3; 
    }

    for (int i = 0; i < MAX_MOUNTS; i++) {
        if (!mount_table[i].is_active) {
            strcpy(mount_table[i].mount_point, mount_point);
            strcpy(mount_table[i].device, device);
            mount_table[i].drive_id = drive_id;
            strcpy(mount_table[i].fs_type, fs_type);
            mount_table[i].is_active = 1;
            
            char msg[128] = "[VFS] Mounted ";
            strcat(msg, device);
            strcat(msg, " on ");
            strcat(msg, mount_point);
            klog(msg);
            
            return 0;
        }
    }
    return -4; 
}

int vfs_unmount(char* mount_point) {
    for (int i = 0; i < MAX_MOUNTS; i++) {
        if (mount_table[i].is_active && strcmp(mount_table[i].mount_point, mount_point) == 0) {
            mount_table[i].is_active = 0;
            return 0;
        }
    }
    return -1;
}

int vfs_resolve_mount(char* path, char** relative_path, int* drive_id) {
    int best_match = -1;
    int best_len = 0;

    for (int i = 0; i < MAX_MOUNTS; i++) {
        if (mount_table[i].is_active) {
            int m_len = strlen(mount_table[i].mount_point);
            if (strncmp(path, mount_table[i].mount_point, m_len) == 0) {
                if (path[m_len] == '/' || path[m_len] == '\0' || strcmp(mount_table[i].mount_point, "/") == 0) {
                    if (m_len > best_len) {
                        best_match = i;
                        best_len = m_len;
                    }
                }
            }
        }
    }

    if (best_match == -1) return -1;

    *drive_id = mount_table[best_match].drive_id;
    if (strcmp(mount_table[best_match].mount_point, "/") == 0) {
        *relative_path = path;
    } else {
        *relative_path = path + best_len;
        if ((*relative_path)[0] == '\0') *relative_path = "/";
    }
    
    return 0;
}

void vfs_init(void) { 
    for(int i = 0; i < MAX_MOUNTS; i++) mount_table[i].is_active = 0;
    klog("[VFS] Virtual File System initialized."); 
    
    int root_mounted = 0;
    for(int i = 0; i < sys_drive_count; i++) {
        char devname[16];
        
        if (sys_drives[i].type == DRIVE_TYPE_AHCI || sys_drives[i].type == DRIVE_TYPE_RAMDISK) 
            sprintf(devname, "/dev/sd%c", 'a' + i);
        else 
            sprintf(devname, "/dev/hd%c", 'a' + i);
        
        if (vfs_mount(devname, "/", "fat32") == 0) {
            root_mounted = 1;
            break;
        }
    }
    
    if(!root_mounted) klog("[VFS] WARNING: Could not find any FAT32 drive to mount as Root (/)!");
}

int vfs_read(char* path, uint8_t* buffer) {    
    if (strcmp(path, "/dev/stdin") == 0) { buffer[0] = (uint8_t)getch(); buffer[1] = '\0'; return 1; }
    if (strcmp(path, "/dev/urandom") == 0 || strcmp(path, "/dev/random") == 0) { buffer[0] = (uint8_t)(random() % 256); return 1; }
    if (strcmp(path, "/dev/zero") == 0) { buffer[0] = 0; return 1; }
    if (strcmp(path, "/dev/null") == 0 || strcmp(path, "/dev/stdout") == 0 || strcmp(path, "/dev/stderr") == 0) return 0;
    
    if (strncmp(path, "/proc/", 6) == 0) {
        char buf[1024]; buf[0] = '\0';
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
        else if (strcmp(path, "/proc/klog") == 0) {
            extern char sys_log_buffer[];
            extern int sys_log_pos;
            strcpy((char*)buffer, sys_log_buffer);
            return sys_log_pos;
        }
        
        else if (strcmp(path, "/proc/disks") == 0) {
            strcpy(buf, "NAME\tTYPE\tDESCRIPTION\n");
            for (int i = 0; i < sys_drive_count; i++) {
                char line[128];
                char dev_name[8];
                sprintf(dev_name, "sd%c", 'a' + i);
                
                char* type_str = "UNK ";
                if (sys_drives[i].type == DRIVE_TYPE_AHCI) type_str = "AHCI";
                else if (sys_drives[i].type == DRIVE_TYPE_ATA) type_str = "IDE ";
                else if (sys_drives[i].type == DRIVE_TYPE_RAMDISK) type_str = "RAM "; 
                
                sprintf(line, "%s\t%s\t%s\n", dev_name, type_str, sys_drives[i].name);
                strcat(buf, line);
            }
        }
        
        int len = strlen(buf);
        strcpy((char*)buffer, buf);
        return len;
    }

    int drive_id; char* rel_path;
    if (vfs_resolve_mount(path, &rel_path, &drive_id) == 0) {
        return fat32_read_file(drive_id, rel_path, buffer);
    }
    return -1;
}


int vfs_write(char* path, uint8_t* buffer, uint32_t size) {
    if (strcmp(path, "/dev/stdout") == 0 || strcmp(path, "/dev/stderr") == 0) {
        for(uint32_t i = 0; i < size; i++) gfx_putc(buffer[i]); return size;
    }
    if (strcmp(path, "/dev/null") == 0) return size;
    if (strncmp(path, "/dev/", 5) == 0 || strncmp(path, "/proc/", 6) == 0) return -1;

    int drive_id; char* rel_path;
    if (vfs_resolve_mount(path, &rel_path, &drive_id) == 0) {
        return fat32_write_file(drive_id, rel_path, buffer, size);
    }
    return -1;
}

int vfs_get_size(char* path) {
    if (strncmp(path, "/dev/", 5) == 0) return 1; 
    if (strcmp(path, "/proc/klog") == 0) {
        extern int sys_log_pos;
        return sys_log_pos;
    }
    if (strncmp(path, "/proc/", 6) == 0) return 512; 
    
    int drive_id; char* rel_path;
    if (vfs_resolve_mount(path, &rel_path, &drive_id) == 0) {
        return fat32_get_file_size(drive_id, rel_path);
    }
    return -1;
}

int vfs_get_attr(char* path, uint8_t* out_type) {
    if (strcmp(path, "/dev") == 0 || strcmp(path, "/dev/") == 0) { *out_type = VFS_ATTR_DIR; return 1; }
    if (strcmp(path, "/proc") == 0 || strcmp(path, "/proc/") == 0) { *out_type = VFS_ATTR_DIR; return 1; }
    
    for (int i = 0; i < num_dev_files; i++) {
        char p[64] = "/dev/"; strcat(p, (char*)dev_files[i]);
        if (strcmp(path, p) == 0) { *out_type = VFS_ATTR_DEV; return 1; }
    }
    
    if (strncmp(path, "/dev/sd", 7) == 0) {
        int idx = path[7] - 'a';
        if (idx >= 0 && idx < sys_drive_count) { *out_type = VFS_ATTR_DEV; return 1; }
    }

    for (int i = 0; i < num_proc_files; i++) {
        char p[64] = "/proc/"; strcat(p, (char*)proc_files[i]);
        if (strcmp(path, p) == 0) { *out_type = VFS_ATTR_FILE; return 1; }
    }
    
    int drive_id; char* rel_path;
    if (vfs_resolve_mount(path, &rel_path, &drive_id) == 0) {
        uint32_t cluster = fat32_get_cluster_for_path(drive_id, rel_path, out_type, NULL);
        if (cluster == 0xFFFFFFFF) return 0;
        return 1;
    }
    return 0;
}

int vfs_readdir(char* path, int index, vfs_dirent_t* out_dirent) {
    if (strcmp(path, "/dev") == 0 || strcmp(path, "/dev/") == 0) {
        if (index < num_dev_files) {
            strcpy(out_dirent->name, (char*)dev_files[index]);
            out_dirent->size = 0; out_dirent->type = VFS_ATTR_DEV; return 1;
        } 
        
        else if (index < num_dev_files + sys_drive_count) {
            int d_idx = index - num_dev_files;
            sprintf(out_dirent->name, "sd%c", 'a' + d_idx);
            out_dirent->size = 0; out_dirent->type = VFS_ATTR_DEV; return 1;
        }
        return 0;
    }
    if (strcmp(path, "/proc") == 0 || strcmp(path, "/proc/") == 0) {
        if (index >= num_proc_files) return 0; 
        strcpy(out_dirent->name, (char*)proc_files[index]);
        out_dirent->size = 0;
        out_dirent->type = VFS_ATTR_FILE; 
        return 1;
    }

    int drive_id; char* rel_path;
    if (vfs_resolve_mount(path, &rel_path, &drive_id) == 0) {
        if (strcmp(path, "/") == 0) {
            if (fat32_readdir(drive_id, rel_path, index, out_dirent) == 1) return 1;
            int fat_count = 0; vfs_dirent_t temp;
            while (fat32_readdir(drive_id, rel_path, fat_count, &temp) == 1) fat_count++;

            if (index == fat_count) { strcpy(out_dirent->name, "dev"); out_dirent->type = VFS_ATTR_DIR; out_dirent->size = 0; return 1; }
            if (index == fat_count + 1) { strcpy(out_dirent->name, "proc"); out_dirent->type = VFS_ATTR_DIR; out_dirent->size = 0; return 1; }
            
            int m_idx = index - (fat_count + 2);
            int m_count = 0;
            for(int i=0; i<MAX_MOUNTS; i++) {
                if(mount_table[i].is_active && strcmp(mount_table[i].mount_point, "/") != 0) {
                    if (m_count == m_idx) {
                        strcpy(out_dirent->name, mount_table[i].mount_point + 1);
                        out_dirent->type = VFS_ATTR_DIR; out_dirent->size = 0; return 1;
                    }
                    m_count++;
                }
            }
            return 0;
        }
        return fat32_readdir(drive_id, rel_path, index, out_dirent);
    }
    return 0;
}

int vfs_mkdir(char* path) {
    if (strncmp(path, "/dev", 4) == 0 || strncmp(path, "/proc", 5) == 0) return -1;
    int drive_id; char* rel_path;
    if (vfs_resolve_mount(path, &rel_path, &drive_id) == 0) {
        return fat32_mkdir(drive_id, rel_path);
    }
    return -1;
}

int vfs_delete(char* path) {
    if (strncmp(path, "/dev", 4) == 0 || strncmp(path, "/proc", 5) == 0) return -1; 
    int drive_id; char* rel_path;
    if (vfs_resolve_mount(path, &rel_path, &drive_id) == 0) {
        return fat32_delete_file(drive_id, rel_path);
    }
    return -1;
}