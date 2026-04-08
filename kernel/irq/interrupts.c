#include "irq/interrupts.h"
#include "drivers/vga/vga.h"
#include "utils/utils.h"
#include "task/task.h"

struct interrupt_frame {
    unsigned int rip;
    unsigned int cs;
    unsigned int rflags;
    unsigned int rsp;
    unsigned int ss;
} __attribute__((packed));

void system_division_handler_c(struct interrupt_frame *frame){
    if (current_task && current_task->id > 1) {
        printf("\n%C[KILL] Process PID %d killed: Division by Zero!%C\n", VGA32_COLOR_RED, current_task->id, VGA32_COLOR_WHITE);
        exit_process();
    } else {
        panic("Kernel Panic: Division by zero inside Kernel!");
    }
}

void page_fault_handler_c(struct registers_t *regs){
    if (current_task && current_task->id > 1) {
        printf("\n%C[SEGFAULT] Process PID %d killed: Access Violation (Page Fault)!%C\n", VGA32_COLOR_RED, current_task->id, VGA32_COLOR_WHITE);
        exit_process();
    } else {
        panic("Kernel Panic: Page Fault inside Kernel!");
    }
}