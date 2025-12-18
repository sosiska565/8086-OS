#include "drivers/vga/vga.h"
#include "drivers/keyboard/keyboardDriver.h"
#include "interrupt/idt/idt.h"
#include "programs/system/console/console.h"
#include "drivers/timer/timer.h"
#include "memory/memory.h"
#include "drivers/speaker/speaker.h"

int $;

struct multiboot_info {
    unsigned long flags;
    unsigned long mem_lower;
    unsigned long mem_upper;
    unsigned long boot_device;
    unsigned long cmdline;
    unsigned long mods_count;
    unsigned long mods_addr;
} __attribute__((packed));


//18.2 one millisecond
void panic(unsigned long err){
    set_text_color(4);
    clear_screen();
    printn("Kernel panic!\n");
    print("Err: ");
    printhex(err);
    printn("System will reboot in 5 seconds...\n");
    unsigned long newTick = get_ticks() + 9100;

    while(get_ticks() < newTick);

    __asm__ volatile (
        "mov $0xFE, %al\n"
        "out %al, $0x64\n"
    );
}

void main_screen(unsigned long magic, unsigned long mb_info_addr){
    clear_screen();

    beep_timed(1000, 10000);

    if(magic != 0x2BADB002){
        panic(magic);
    }

    struct multiboot_info* mbi = (struct multiboot_info*) mb_info_addr;

    if (mbi->flags & 0x01) {
        unsigned long total_mem_kb = mbi->mem_lower + mbi->mem_upper;
        print("Memory stat:\n");
        print("  Lower (conventional): ");
        printnumber(mbi->mem_lower);
        print(" KB\n");
        print("  Upper (extended): ");
        printnumber(mbi->mem_upper);
        print(" KB\n");
        print("  Total: ");
        printnumber(total_mem_kb);
        print(" KB (");
        printnumber(total_mem_kb / 1024);
        print(" MB)\n\n");
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
    clear_screen();
    pic_remap();
    idt_install();
    idt_init();
    timer_install();

    __asm__ volatile("sti");

    heap_init();
    heap_dump();

    main_screen(magic, mb_info_addr);

    int x = 0 / 0;

    $ = console.main();
    print("\n");
    printnumber($);

    return;
}