#include <stdint.h>

#include "drivers/vga/vga.h"
#include "drivers/keyboard/keyboardDriver.h"
#include "idt/idt.h"
#include "system_apps/console/console.h"
#include "drivers/timer/timer.h"
#include "mm/memory.h"
#include "drivers/speaker/speaker.h"
#include "multiboot.h"
#include "drivers/file/initrd.h"
#include "global.h"
#include "fs/fat/fat32.h"
#include "drivers/mouse/mouse.h"
#include "utils/utils.h"
#include "drivers/pci/pci.h"
#include "drivers/video/bga/gfx_console.h"
#include "drivers/video/vesa.h"
#include "task/task.h"
#include "mm/paging.h"
#include "system_apps/initd/initd.h"
#include "drivers/AHCI/AHCI.h"
#include "drivers/file/ATA/ATA.h"
#include "fs/vfs.h"
#include "utils/sysconfig.h"

unsigned short isReadMode = 0;
int $;
char* path = "/";
struct multiboot_info* mbi;

void kmain(unsigned long magic, unsigned long mb_info_addr){
    if(magic != 0x2BADB002){
        panic("Magic value is not correct");
    }

    pic_remap();
    idt_install();
    idt_init();
    timer_install();

    __asm__ volatile("sti");

    mbi = (struct multiboot_info*) mb_info_addr;

    heap_init();
    init_vesa();
    init_gfx_console();

    init_paging();

    printf("Gfx console initialized.\n");

    pci_scan();
    printf("[DEBUG] pci_scan() finished.\n");

    printf("[DEBUG] Starting disk_manager_init()...\n");
    disk_manager_init();
    printf("[DEBUG] disk_manager_init() finished.\n");

    printf("[DEBUG] Starting ahci_init()...\n");
    ahci_init();
    printf("[DEBUG] ahci_init() finished.\n");

    printf("[DEBUG] Starting fat32_init()...\n");
    fat32_init();
    printf("[DEBUG] fat32_init() finished.\n");

    vfs_init();

    srand(get_ticks());

    printf("Window manager initialized.\n");

    init_tasking();
    printf("Tasking initialized.\n");

    printf("Please press any key to continue...\n");
    mouse_init();

    create_process(initd, 0, 0, "initd", kernel_dir);

    while(1) {
        __asm__ volatile("hlt");
    }
}