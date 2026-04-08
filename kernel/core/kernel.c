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

    klog("[INIT] Kernel started booting.");

    heap_init();
    klog("[INIT] Heap memory manager initialized.");

    init_vesa();
    init_gfx_console();
    klog("[INIT] VESA Graphics and BGA Console initialized.");

    init_paging();
    klog("[INIT] Paging and Virtual Memory enabled.");

    pci_scan();
    klog("[INIT] PCI Bus scan finished.");

    disk_manager_init();
    klog("[INIT] ATA/IDE Disk manager finished.");

    ahci_init();
    klog("[INIT] AHCI SATA driver initialized.");

    fat32_init();
    klog("[INIT] FAT32 File System mounted.");

    vfs_init();

    srand(get_ticks());

    init_tasking();
    klog("[INIT] Multitasking engine initialized.");

    mouse_init();
    klog("[INIT] PS/2 Mouse driver initialized.");

    klog("[INIT] Spawning Initd process...");
    create_process(initd, 0, 0, "initd", kernel_dir);

    while(1) {
        __asm__ volatile("hlt");
    }
}