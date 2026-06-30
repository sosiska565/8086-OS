/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/userland/apps/testsig.c
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */

#include <oslib.h>
#include <signal.h>

void my_handler(int sig) {
    set_color(COLOR_YELLOW, COLOR_BLACK);
    printf("\nGot signal %d! I will NOT die!\n", sig);
    set_color(COLOR_WHITE, COLOR_BLACK);
}

int main() {
    printf("Setting up SIGINT handler...\n");
    signal(SIGINT, my_handler);
    
    printf("Press Ctrl+C to test. I will run forever.\n");
    while(1) {
        printf(".");
        uint32_t t = get_ticks() + 1000;
        while(get_ticks() < t) yield();
    }
    return 0;
}
