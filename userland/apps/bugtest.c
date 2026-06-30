/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/userland/apps/bugtest.c
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */

#include <oslib.h>
#include "libgui.h"

void print_header(char *text) {
    set_color(COLOR_LIGHT_CYAN, COLOR_BLACK);
    printf("\n=== %s ===\n", text);
    set_color(COLOR_WHITE, COLOR_BLACK);
}

void print_result(char *test_name, int passed) {
    printf("%s ", test_name);
    
    
    int dots = 40 - strlen(test_name);
    for(int i = 0; i < dots; i++) printf(".");
    
    if (passed) {
        set_color(COLOR_LIGHT_GREEN, COLOR_BLACK);
        printf(" [ SURVIVED ]\n");
    } else {
        set_color(COLOR_LIGHT_RED, COLOR_BLACK);
        printf(" [ KERNEL VULNERABILITY! ]\n");
    }
    set_color(COLOR_WHITE, COLOR_BLACK);
}

int main(int argc, char** argv) {
    set_color(COLOR_LIGHT_MAGENTA, COLOR_BLACK);
    printf("========================================\n");
    printf("      8086-OS KERNEL TORTURE TEST       \n");
    printf("========================================\n");
    set_color(COLOR_WHITE, COLOR_BLACK);

    int fd;
    int ret;

    
    print_header("1. KERNEL POINTER SHIELD (MEMORY)");
    
    
    ret = read(0, NULL, 10);
    print_result("Read to NULL pointer", ret == -1);

    
    ret = write(1, (void*)0x00100000, 10);
    print_result("Write from Kernel Memory", ret == -1);

    
    ret = uname((void*)0xFFFFFFFF);
    print_result("Syscall with 0xFFFFFFFF pointer", ret == -1);

    
    ret = open((char*)0x00001234, O_RDONLY);
    print_result("Open file with Kernel pointer", ret == -1);

    
    print_header("2. FILE SYSTEM ABUSE");

    
    ret = read(999, "buf", 10);
    print_result("Read from invalid FD (999)", ret == -1 || ret == 0);

    
    char long_path[300];
    for(int i=0; i<299; i++) long_path[i] = 'A';
    long_path[299] = '\0';
    ret = open(long_path, O_RDONLY);
    print_result("Open 300-character path", ret == -1);

    
    print_header("3. MEMORY ALLOCATOR (HEAP)");

    
    void *ptr1 = malloc(0xFFFFFFFF);
    print_result("malloc(0xFFFFFFFF)", ptr1 == NULL);

    
    
    free((void*)0x00100000); 
    free(NULL);
    print_result("Freeing Kernel & NULL pointers", 1); 

    
    print_header("4. PROCESS & IPC");

    
    ret = kill(0, 9);
    
    print_result("Kill Kernel (PID 0)", ret <= 0); 

    
    ret = shm_get(0, 0xFFFFFFFF);
    print_result("SHM allocate 4GB", ret == -1 || ret == 0);

    
    print_header("5. WINDOW MANAGER ABUSE");

    
    gui_window_t *bad_win1 = gui_create_window("Negative", -100, -500);
    print_result("Window with negative size", bad_win1 == NULL || bad_win1->w <= 0);
    if(bad_win1) gui_destroy_window(bad_win1);

    
    gui_window_t *bad_win2 = gui_create_window("Massive", 20000, 20000);
    print_result("20000x20000 Window creation", bad_win2 == NULL);
    if(bad_win2) gui_destroy_window(bad_win2);

    
    print_header("6. APPLICATION CRASH HANDLING");
    printf("The OS shield passed all tests!\n");
    printf("Now, we will intentionally crash this app (Division by Zero).\n");
    printf("The OS MUST catch it, print a Crash Dump, and survive.\n\n");
    
    printf("Press [ENTER] to execute Division by Zero...\n");
    while(getc() != '\n');

    
    volatile int zero = 0;
    volatile int crash = 100 / zero;

    
    printf("Result: %d. Wait, I shouldn't be alive!\n", crash);

    return 0;
}
