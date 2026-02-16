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

#include "drivers/file/ATA/ATA.h"

unsigned short isReadMode;
int $;
char* path = "/";
struct multiboot_info* mbi;

void kmain(unsigned long magic, unsigned long mb_info_addr){
    //init
    if(magic != 0x2BADB002){
        panic("Magic value is not correct");
    }

    pic_remap();
    idt_install();
    idt_init();
    timer_install();
    

    __asm__ volatile("sti");

    mbi = (struct multiboot_info*) mb_info_addr;

    //run setup
    init_vesa();
    init_gfx_console();
    printf("gfx console initialized successfully\n");
    printf("vesa initialized successfully\n");
    printf("timer initialized successfully\n");
    printf("idt initialized successfully\n");
    //init
    
    heap_init();
    printf("heap initialized successfully\n");
    heap_dump();
    init_paging();
    fat32_init();
    printf("fat32 initialized successfully\n");
    srand(get_ticks());

    pci_scan();
    
    printf("Please press any key to continue...");
    wait_scancode();

    wm_init();
    init_tasking();
    // mouse_init();
    //printf("mouse initialized successfully\n");

    setup.main();

    create_process((void (*)(int, char**))console.main, 0, 0);

    while(1) {
        __asm__ volatile("hlt");
    }
    return;
}