#ifndef MULTIBOOT_H
#define MULTIBOOT_H

struct multiboot_module {
    unsigned long mod_start;
    unsigned long mod_end;
    unsigned long string;
    unsigned long reserved;
} __attribute__((packed));

struct multiboot_info {
    unsigned long flags;
    unsigned long mem_lower;
    unsigned long mem_upper;
    unsigned long boot_device;
    unsigned long cmdline;
    unsigned long mods_count;
    unsigned long mods_addr;
} __attribute__((packed));

#endif