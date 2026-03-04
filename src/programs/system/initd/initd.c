#include "programs/system/initd/initd.h"
#include "multitask/task.h"
#include "programs/system/console/console.h"
#include "graphics/taskbar.h"
#include "drivers/vga/vga.h"
#include "graphics/interface.h"
#include "programs/system/setup/setup.h"
#include "programs/system/console/system.h"
#include "global.h"

void initd(int argc, char **argv){
    if(isReadMode == 0){
        cmd_readsystemcfg(0);
    }

    create_process((void (*)(int, char**))draw_interface, 0, 0, "interface", kernel_dir);
    create_process((void (*)(int, char**))draw_taskbar, 0, 0, "taskbar", kernel_dir);
    create_process((void (*)(int, char**))setup.main, 0, 0, "setup", kernel_dir);

    while(1){
        cleanup_zombies();
        __asm__ volatile("hlt");
    };
}