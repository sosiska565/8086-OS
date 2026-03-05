#include "system_apps/initd/initd.h"
#include "task/task.h"
#include "system_apps/console/console.h"
#include "graphics/taskbar.h"
#include "drivers/vga/vga.h"
#include "graphics/interface.h"
#include "system_apps/setup/setup.h"
#include "system_apps/console/system.h"
#include "global.h"
#include "utils/sysconfig.h"

void initd(int argc, char **argv){
    sysconfig_init();

    create_process((void (*)(int, char**))draw_interface, 0, 0, "interface", kernel_dir);
    create_process((void (*)(int, char**))draw_taskbar, 0, 0, "taskbar", kernel_dir);
    // create_process((void (*)(int, char**))setup.main, 0, 0, "setup", kernel_dir);

    while(1){
        cleanup_zombies();
        __asm__ volatile("hlt");
    };
}