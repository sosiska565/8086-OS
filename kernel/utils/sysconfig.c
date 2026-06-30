/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/kernel/utils/sysconfig.c
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */

#include "utils/sysconfig.h"
#include "utils/utils.h"
#include "fs/vfs.h"
#include "global.h"
#include "mm/memory.h"
#include "drivers/vga/vga.h"

Config *global_cfg = 0;
static uint8_t *current_cfg_buffer = 0;

void sysconfig_reload(void) {
    int file_size = vfs_get_size("/kernel.cfg");
    
    
    if(file_size <= 0) {
        char* def = "PROMPT_USER_COLOR=11\nPROMPT_HOST_COLOR=10\nPROMPT_PATH_COLOR=9\nPATH=/path\n";
        vfs_write("/kernel.cfg", (uint8_t*)def, strlen(def));
        file_size = strlen(def);
    }
    
    uint8_t *file_buffer = (uint8_t*)kmalloc_a(file_size + 1);
    vfs_read("/kernel.cfg", file_buffer);
    file_buffer[file_size] = '\0';
    
    if (global_cfg) config_free(global_cfg);
    if (current_cfg_buffer) kfree_a(current_cfg_buffer);
    global_cfg = config_parse((char *)file_buffer);

    current_cfg_buffer = file_buffer;
}

void sysconfig_init(void) { sysconfig_reload(); }
