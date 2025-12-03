#include "drivers/vga/vga.h"
#include "drivers/keyboard/keyboardDriver.h"
#include "interrupt/idt/idt.h"
#include "programs/system/console/console.h"
#include "drivers/timer/timer.h"

int $;

void panic(char *err){
    set_text_color(4);
    clear_screen();
    printn("Kernel panic!\n");
    print("Err: ");
    print(err);
    printn("System will reboot in 5 seconds...\n");
    unsigned long newTick = get_ticks() + 91;

    while(get_ticks() < newTick);

    __asm__ volatile (
        "mov $0xFE, %al\n"
        "out %al, $0x64\n"
    );
}

void main_screen(void){
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
    printn("\nWelcome to 8086-OS!");
    unsigned long newTick = get_ticks() + 36;
    while(get_ticks() < newTick);
    clear_screen();
}

struct multiboot_info {
    unsigned long flags;
    unsigned long mem_lower;
    unsigned long mem_upper;
    unsigned long boot_device;
    unsigned long cmdline;
    unsigned long mods_count;
    unsigned long mods_addr;
} __attribute__((packed));

void kmain(unsigned long magic, unsigned long mb_info_addr){
    clear_screen();

    pic_remap();
    idt_install();
    idt_init();

    __asm__ volatile("sti");

    main_screen();

    if(magic != 0x2BADB002){
        panic("0x2BADB002");
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

    parse_str("Hello world man!!");
    $ = console.main();
    print("\n");
    printnumber($);

    return;
}