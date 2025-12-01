#include "drivers/vga/vga.h"
#include "interrupt/idt/idt.h"

void kmain(void){
    clear_screen();
    pic_remap();
    idt_install();
    idt_init();

    __asm__ volatile("sti");

    while (1)
    {
        print("> ");
    }
    

    return;
}