#include "drivers/vga/vga.h"
#include "drivers/keyboard/keyboardDriver.h"
#include "interrupt/idt/idt.h"
#include "programs/system/console/console.h"

int $;

void kmain(void){
    clear_screen();
    pic_remap();
    idt_install();
    idt_init();

    __asm__ volatile("sti");

    $ = console.main();
    print("\n");
    printnumber($);

    return;
}