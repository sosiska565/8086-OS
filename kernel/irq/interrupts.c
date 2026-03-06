#include "irq/interrupts.h"
#include "drivers/vga/vga.h"
#include "utils/utils.h"

struct interrupt_frame {
    unsigned int rip;
    unsigned int cs;
    unsigned int rflags;
    unsigned int rsp;
    unsigned int ss;
} __attribute__((packed));

void system_division_handler_c(struct interrupt_frame *frame){
    print_info("ERROR", "Division by zero.\n", VGA_COLOR_RED, VGA_COLOR_LIGHT_GREY);
    print_info("INFO", "Location: ", VGA_COLOR_YELLOW, VGA_COLOR_LIGHT_GREY);
    printhex(frame->rip);
    printf("\n");
    
    frame->rip += 2;
}

void page_fault_handler_c(registers_t *regs){
    panic_with_regs(regs, "PAGE FAULT");
}