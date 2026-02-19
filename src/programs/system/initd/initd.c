#include "programs/system/initd/initd.h"
#include "multitask/task.h"
#include "programs/system/console/console.h"
#include "graphics/taskbar.h"
#include "drivers/vga/vga.h"
#include "graphics/interface.h"

void initd(int argc, char **argv){
    create_process((void (*)(int, char**))draw_interface, 0, 0, "interface");
    create_process((void (*)(int, char**))draw_taskbar, 0, 0, "taskbar");
    create_process((void (*)(int, char**))console.main, 0, 0, "console");

    while(1){
        cleanup_zombies();
        __asm__ volatile("hlt");
    };
}