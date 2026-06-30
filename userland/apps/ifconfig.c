/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/userland/apps/ifconfig.c
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */

#include "oslib.h"

int main(int argc, char** argv) {
    int sz = get_file_size("/proc/net");
    
    if (sz <= 0) {
        set_color(COLOR_RED, COLOR_BLACK);
        printf("ifconfig: Network interface not found or driver failed.\n");
        set_color(COLOR_LIGHT_GRAY, COLOR_BLACK);
        return 1;
    }

    uint8_t* buf = malloc(sz + 1);
    read_file("/proc/net", buf);
    buf[sz] = '\0';

    set_color(COLOR_LIGHT_CYAN, COLOR_BLACK);
    printf("=== Network Interfaces ===\n");
    set_color(COLOR_WHITE, COLOR_BLACK);
    
    printf("%s", buf);

    
    if (strstr((char*)buf, "IP Address  : 0.0.0.0") != NULL) {
        set_color(COLOR_YELLOW, COLOR_BLACK);
        printf("\nWarning: Interface is UP, but waiting for DHCP offer...\n");
    } else {
        set_color(COLOR_LIGHT_GREEN, COLOR_BLACK);
        printf("\nNetwork is connected and ready!\n");
    }

    set_color(COLOR_LIGHT_GRAY, COLOR_BLACK);
    free(buf);
    return 0;
}
