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
#include "drivers/AHCI/AHCI.h"
#include "drivers/file/ATA/ATA.h"

unsigned short isReadMode = 1;
int $;
char* path = "/";
struct multiboot_info* mbi;

uint32_t taskbar_color = 0x191970;
uint32_t window_border_color = VGA32_COLOR_DARK_GREY;
uint32_t window_active_border_color = VGA32_COLOR_BLUE;


uint8_t key_kill = 0x10;          
uint8_t key_focus = 0x0F;         
uint8_t key_console = 0x14;       
uint8_t key_layout = 0x39;        
uint8_t key_fullscreen = 0x57;    
uint8_t key_ws_left = 0x4B;       
uint8_t key_ws_right = 0x4D;      
uint8_t key_resize_left = 0x4B;   
uint8_t key_resize_right = 0x4D;  
uint8_t key_resize_up = 0x48;     
uint8_t key_resize_down = 0x50;   

int wm_gaps = 15;
int max_grid_cols = 2; 
int current_workspace = 0;

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

    srand(get_ticks());

    wm_init();
    printf("Window manager initialized.\n");

    init_tasking();
    printf("Tasking initialized.\n");

    printf("Please press any key to continue...\n");
    getch();

    create_process(initd, 0, 0, "initd", kernel_dir);

    while(1) {
        __asm__ volatile("hlt");
    }
}