#include "system_apps/initd/initd.h"
#include "task/task.h"
#include "system_apps/console/console.h"
#include "drivers/vga/vga.h"
#include "utils/sysconfig.h"

void initd(int argc, char **argv){
    sysconfig_init();

    printf("\n=== 8086-OS Kernel Booted ===\n");
    printf("Mounting root filesystem...\n");
    
    int sh_pid = spawn_process("/path/sh.elf", NULL, NULL);
    
    if (sh_pid < 0) {
        printf("%C[WARN] Failed to start Userland Login (/path/login.bin)!%C\n", VGA32_COLOR_YELLOW, VGA32_COLOR_WHITE);
        printf("Falling back to Kernel Recovery Console...\n\n");
        create_process((void (*)(int, char**))console.main, 0, 0, "ksh", kernel_dir);
    } else {
        printf("%C[OK] Userland Environment started successfully.%C\n\n", VGA32_COLOR_GREEN, VGA32_COLOR_WHITE);
        wait_process(sh_pid);
    }

    while(1){
        cleanup_zombies();
        __asm__ volatile("hlt");
    }
}