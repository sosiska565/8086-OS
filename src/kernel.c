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

#include "drivers/file/ATA/ATA.h"

unsigned short isReadMode;
int $;
char* path = "/";
struct multiboot_info* mbi;

void main_screen(unsigned long magic, unsigned long mb_info_addr, struct multiboot_info* mbi){
    clear_screen();

    beep_timed(1000, 10000);

    if(magic != 0x2BADB002){
        panic("Magic value is not correct");
    }

    set_text_color(1);
    printn(" ::::::::   :::::::   ::::::::   ::::::::                 ::::::::   ::::::::");
    printn(":+:    :+: :+:   :+: :+:    :+: :+:    :+:               :+:    :+: :+:    :+:");
    printn("+:+    +:+ +:+  :+:+ +:+    +:+ +:+                      +:+    +:+ +:+");
    printn(" +#++:++#  +#+ + +:+  +#++:++#  +#++:++#+  +#++:++#++:++ +#+    +:+ +#++:++#++");
    set_text_color(9);
    printn("+#+    +#+ +#+#  +#+ +#+    +#+ +#+    +#+               +#+    +#+        +#+");
    printn("#+#    #+# #+#   #+# #+#    #+# #+#    #+#               #+#    #+# #+#    #+#");
    printn(" ########   #######   ########   ########                 ########   ########");
    set_text_color(7);

    printn("\nWelcome to 8086-OS!\n");
    printn("Press any key...\n");
    getch();
    clear_screen();
}

void kmain(unsigned long magic, unsigned long mb_info_addr){
    //init
    clear_screen();
    pic_remap();
    idt_install();
    idt_init();
    timer_install();

    __asm__ volatile("sti");

    mbi = (struct multiboot_info*) mb_info_addr;

    main_screen(magic, mb_info_addr, mbi);

    //run setup

    //init
    heap_init();
    heap_dump();
    fat32_init();
    srand(get_ticks());
    // mouse_init(); в пизду мышку бля

    //
    setup.main();
    $ = console.main();
    print("\n");
    printnumber($);

    return;
}