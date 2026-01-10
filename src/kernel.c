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
#include "drivers/video/bga/bga.h"
#include "drivers/video/bga/gfx_console.h"

#include "drivers/file/ATA/ATA.h"

unsigned short isReadMode;
int $;
char* path = "/";
struct multiboot_info* mbi;
int graphic_mode = 1;

void kmain(unsigned long magic, unsigned long mb_info_addr){
    //init
    if(magic != 0x2BADB002){
        panic("Magic value is not correct");
    }

    clear_screen();
    pic_remap();
    idt_install();
    idt_init();
    timer_install();

    __asm__ volatile("sti");

    mbi = (struct multiboot_info*) mb_info_addr;

    //run setup

    //init
    heap_init();
    heap_dump();
    fat32_init();
    srand(get_ticks());
    // mouse_init(); в пизду мышку бля
    
    setup.main();

    pci_scan();

    if(graphic_mode == 2 && isReadMode == 1) init_bga(800, 600);
    console.main();
    return;
}