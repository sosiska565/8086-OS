/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/userland/apps/ls.c
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */

#include <oslib.h>

int main(int argc, char** argv) {
    vfs_dirent_t entry; 
    int idx = 0;
    char* target = (argc > 1) ? argv[1] : ".";
    
    while (readdir(target, idx++, &entry) == 1) {
        if (entry.type == VFS_ATTR_DIR) { 
            set_color(COLOR_LIGHT_BLUE, COLOR_BLACK); 
            printf("[DIR]  "); 
        }
        else if (entry.type == VFS_ATTR_DEV) { 
            set_color(COLOR_YELLOW, COLOR_BLACK); 
            printf("[DEV]  "); 
        }
        else { 
            set_color(COLOR_LIGHT_GRAY, COLOR_BLACK); 
            printf("[FILE] "); 
        }
        
        printf(entry.name);
        if (entry.type == VFS_ATTR_FILE) { 
            printf("\t%d B", entry.size); 
        }
        printf("\n");
    }
    
    set_color(COLOR_LIGHT_GRAY, COLOR_BLACK);
    return 0;
}
