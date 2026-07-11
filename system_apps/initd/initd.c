/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/system_apps/initd/initd.c
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */


#include "system_apps/initd/initd.h"
#include "task/task.h"
#include "drivers/vga/vga.h"
#include "fs/vfs.h"
#include "kernel/utils/utils.h"
#include "kernel/mm/memory.h"

#define INITD_PATH "/config/initd/initd.cfg"

void initd(int argc, char **argv) {
    printf("\n=== 8086-OS Kernel Booted ===\n");
    printf("Mounting root filesystem...\n");
    
    uint8_t config_file[512];
    memset(config_file, 0, sizeof(config_file));
    
    
    if (vfs_ensure_file_exists(INITD_PATH) == -1) {
        panic("code 7");
    }

    
    int bytes_read = vfs_read(INITD_PATH, config_file);

    if (bytes_read > 0) {
        
        
        int max_apps = 32;
        char **apps_to_start = (char **)kmalloc(max_apps * sizeof(char *));
        int app_count = 0;

        
        int write_idx = 0;
        for (int i = 0; i < bytes_read && i < sizeof(config_file); i++) {
            char c = config_file[i];
            if (c != ' ' && c != '\n' && c != '\r') {
                config_file[write_idx++] = c;
            }
        }
        config_file[write_idx] = '\0'; 

        
        if (write_idx > 0) {
            apps_to_start[app_count++] = (char *)&config_file[0]; 

            for (int i = 0; i < write_idx; i++) {
                if (config_file[i] == ',') {
                    config_file[i] = '\0'; 
                    
                    
                    if (i + 1 < write_idx && app_count < max_apps) {
                        apps_to_start[app_count++] = (char *)&config_file[i + 1];
                    }
                }
            }
        }

        
        for (int i = 0; i < app_count; i++) {
            
            if (strlen(apps_to_start[i]) > 0) { 
                spawn_process(apps_to_start[i], NULL);
            }
        }

        kfree(apps_to_start);
        
    }

    while (1) {
        cleanup_zombies();
        __asm__ volatile("hlt");
    }
}
