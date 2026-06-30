/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/userland/apps/fetch.c
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */

#include "oslib.h"

int main(int argc, char** argv) {
    os_version_info os_info;
    
    if (uname(&os_info) == 0) {
        uint32_t used_mem, total_mem;
        get_mem_info(&used_mem, &total_mem);
        
        set_color(COLOR_LIGHT_CYAN, COLOR_BLACK);
        printf("      .---.       "); set_color(COLOR_WHITE, COLOR_BLACK); printf("OS: "); set_color(COLOR_LIGHT_BLUE, COLOR_BLACK); printf("%s %s\n", os_info.sysname, os_info.release);
        set_color(COLOR_LIGHT_CYAN, COLOR_BLACK);
        printf("    /       \\     "); set_color(COLOR_WHITE, COLOR_BLACK); printf("Arch: "); set_color(COLOR_LIGHT_GRAY, COLOR_BLACK); printf("%s\n", os_info.machine);
        set_color(COLOR_LIGHT_CYAN, COLOR_BLACK);
        printf("   |  8086   |    "); set_color(COLOR_WHITE, COLOR_BLACK); printf("Kernel Built: "); set_color(COLOR_LIGHT_GRAY, COLOR_BLACK); printf("%s\n", os_info.version);
        set_color(COLOR_LIGHT_CYAN, COLOR_BLACK);
        printf("   |   OS    |    "); set_color(COLOR_WHITE, COLOR_BLACK); printf("RAM: "); set_color(COLOR_LIGHT_GRAY, COLOR_BLACK); printf("%d MB / %d MB\n", used_mem / (1024*1024), total_mem / (1024*1024));
        set_color(COLOR_LIGHT_CYAN, COLOR_BLACK);
        printf("    \\       /     "); set_color(COLOR_WHITE, COLOR_BLACK); printf("Uptime: "); set_color(COLOR_LIGHT_GRAY, COLOR_BLACK); printf("%d sec\n", get_ticks() / 1000);
        set_color(COLOR_LIGHT_CYAN, COLOR_BLACK);
        printf("      '---'       \n");
    } else {
        printf("Failed to get OS info.\n");
    }
    
    set_color(COLOR_LIGHT_GRAY, COLOR_BLACK);
    return 0;
}
