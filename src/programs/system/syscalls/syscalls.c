#include "programs/system/syscalls/syscalls.h"
#include "drivers/vga/vga.h"
#include "drivers/keyboard/keyboardDriver.h"
#include "memory/memory.h"
#include "utils/utils.h"
#include "drivers/timer/timer.h"

//THIS IS OS API
//input: eax 0, ebx char = print char
//input: eax 1 = exit program
//input: eax 2, output: eax = getting char

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
    else if(regs->eax == 10) {
        draw_simple_box((char**)regs->ebx, (char*)regs->ecx, (uint8_t)regs->edx); 
    }
    else if(regs->eax == 11) set_cursor_position((unsigned int)regs->ebx, (unsigned int)regs->ecx);
    else {
        print("Unknown syscall!\n");
    }
}