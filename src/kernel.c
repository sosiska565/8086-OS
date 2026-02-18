#include <stdint.h>

#include "drivers/vga/vga.h"
#include "drivers/keyboard/keyboardDriver.h"
#include "interrupt/idt/idt.h"
#include "programs/system/console/console.h"
#include "drivers/timer/timer.h"
#include "memory/memory.h"
#include "drivers/speaker/speaker.h"
#include "multiboot.h"
#include "drivers/file/initrd.h"
#include "global.h"
#include "programs/system/setup/setup.h"
#include "fs/fat/fat32.h"
#include "drivers/mouse/mouse.h"
#include "utils/utils.h"
#include "drivers/pci/pci.h"
#include "drivers/video/bga/gfx_console.h"
#include "drivers/video/vesa.h"
#include "drivers/video/graphics.h"
#include "multitask/task.h"
#include "memory/paging.h"
#include "programs/system/initd/initd.h"

#include "drivers/file/ATA/ATA.h"

unsigned short isReadMode;
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
    printf("Vesa initialized.\n");
    printf("Heap initialized.\n");

    init_paging();
    printf("Paging initialized.\n");
    init_gfx_console();
    printf("Gfx console initialized.\n");

    fat32_init();
    printf("FAT32 initialized.\n");

    srand(get_ticks());

    pci_scan();

    wm_init();
    printf("Window manager initialized.\n");

    init_tasking();
    printf("Tasking initialized.\n");

    printf("Please press any key to continue...");
    getch();
    
    setup.main();

    create_process(initd, 0, 0, "initd");

    while(1) {
        __asm__ volatile("hlt");
    }
}
