/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/userland/apps/lsblk.c
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */

#include <oslib.h>

int main(int argc, char** argv) {
    int sz = get_file_size("/proc/disks");
    if (sz <= 0) {
        set_color(COLOR_RED, COLOR_BLACK);
        printf("lsblk: failed to read /proc/disks\n");
        set_color(COLOR_LIGHT_GRAY, COLOR_BLACK);
        return 1;
    }

    uint8_t* buf = malloc(sz + 1);
    read_file("/proc/disks", buf);
    buf[sz] = '\0';

    set_color(COLOR_LIGHT_CYAN, COLOR_BLACK);
    printf("=== Block Devices ===\n");
    set_color(COLOR_WHITE, COLOR_BLACK);
    
    printf("%s", buf); 

    set_color(COLOR_LIGHT_GRAY, COLOR_BLACK);
    free(buf);
    return 0;
}
