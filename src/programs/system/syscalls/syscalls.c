#include "programs/system/syscalls/syscalls.h"
#include "drivers/vga/vga.h"
#include "drivers/keyboard/keyboardDriver.h"
#include "memory/memory.h"
#include "utils/utils.h"
#include "drivers/timer/timer.h"
#include "fs/fat/fat32.h"

void syscall_handler_c(struct registers *regs){
    if(regs->eax == 0) print_char((char)regs->ebx);
    else if(regs->eax == 1) return;
    else if(regs->eax == 2) regs->eax = (uint32_t)getch();
    else if(regs->eax == 3) gets((char*)regs->ebx, (int)regs->ecx);
    else if(regs->eax == 4) regs->eax = (uint32_t)kmalloc((size_t)regs->ebx);
    else if(regs->eax == 5) kfree((void*)regs->ebx);
    else if(regs->eax == 6) clear_screen();
    else if(regs->eax == 7) print_char_colored((char)regs->ebx, (uint8_t)regs->ecx);
    else if(regs->eax == 8) regs->eax = random();
    else if(regs->eax == 9) printnumber((int)regs->ebx);
    else if(regs->eax == 10) regs->eax = (int)fat32_read_file((char *)regs->ebx, (uint8_t*)regs->ecx);
    else if(regs->eax == 11) set_cursor_position((unsigned int)regs->ebx, (unsigned int)regs->ecx);
    else if(regs->eax == 12) set_background_color((vga_color_t)regs->ebx);
    else if(regs->eax == 13) set_text_color((vga_color_t)regs->ebx);
    else if(regs->eax == 14) get_cursor_xy(&regs->ebx, &regs->ecx);
    else if(regs->eax == 15) regs->eax = (vga_color_t)get_current_color();
    else if(regs->eax == 16) set_current_color((vga_color_t)regs->ebx);
    else if(regs->eax == 17) printhex((unsigned int)regs->ebx);
    else if(regs->eax == 18) regs->eax = (int)fat32_get_file_size((char *)regs->ebx);
    else if(regs->eax == 19) regs->eax = wait_scancode();
    else if(regs->eax == 20) regs->eax = fat32_write_file((char*)regs->ebx, (uint8_t*)regs->ecx, (uint32_t)regs->edx);
    else if(regs->eax == 21) regs->eax = scancode_to_char((uint8_t)regs->ebx);
    else {
        print("Unknown syscall!\n");
    }
}