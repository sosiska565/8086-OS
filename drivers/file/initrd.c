/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/drivers/file/initrd.c
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */

#include "drivers/file/initrd.h"
#include "drivers/vga/vga.h"
#include "drivers/file/ATA/ATA.h"
#include "utils/utils.h"

extern uint32_t ramdisk_start;
extern uint32_t ramdisk_size;

void initrd_files(struct multiboot_info* mbi) {
    if(!(mbi->flags & 0x08)) {
        klog("[INITRD] No modules loaded via GRUB.");
        return;
    }

    struct multiboot_module* mod = (struct multiboot_module*) mbi->mods_addr;

    for(unsigned int i = 0; i < mbi->mods_count; i++) {
        char* module_name = (char*)mod[i].string;        
        
        int len = strlen(module_name);
        if (strcmp(module_name, "disk.img") == 0) {
            ramdisk_start = mod[i].mod_start;
            ramdisk_size = mod[i].mod_end - mod[i].mod_start;
            
            
            if (sys_drive_count < MAX_SYS_DRIVES) {
                
                for (int j = sys_drive_count; j > 0; j--) {
                    sys_drives[j] = sys_drives[j - 1];
                }
                
                sys_drives[0].type = DRIVE_TYPE_RAMDISK;
                strcpy(sys_drives[0].name, "LiveUSB Ramdisk");
                sys_drive_count++;
                
                klog("[INITRD] Ramdisk loaded into memory successfully.");
            }
        }
    }
}
