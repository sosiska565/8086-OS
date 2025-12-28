#include "programs/system/syscalls/syscalls.h"
#include "drivers/vga/vga.h"
#include "drivers/keyboard/keyboardDriver.h"
#include "memory/memory.h"

//THIS IS OS API
//input: eax 0, ebx char = print char
//input: eax 1 = exit program
//input: eax 2, output: eax = getting char

void syscall_handler_c(struct registers *regs){
    if(regs->eax == 0) print_char((char)regs->ebx);
    else if(regs->eax == 1) return;
    else if(regs->eax == 2) regs->eax = (uint32_t)getch();
    else if(regs->eax == 3) gets(regs->ebx, regs->ecx);
    else if(regs->eax == 4) regs->eax = kmalloc(regs->ebx);
    else if(regs->eax == 5) kfree(regs->ebx);
    else if(regs->eax == 6) clear_screen();
    else {
        print("Unknown syscall!\n");
    }
}