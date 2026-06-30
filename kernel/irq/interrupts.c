/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/kernel/irq/interrupts.c
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */

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
        char dump[128];
        sprintf(dump, "[CRASH] Process '%s' (PID %d) Killed: Division by Zero!", current_task->name, current_task->id);
        klog(dump);
        klog_save();
        exit_process();
    } else {
        panic("Kernel Panic: Division by zero inside Kernel!");
    }
}

void page_fault_handler_c(struct registers_t *regs) {
    uint32_t cr2;
    __asm__ volatile("mov %%cr2, %0" : "=r"(cr2)); 

    if (current_task && current_task->id > 1) {
        
        char dump[256];
        sprintf(dump, "[CRASH DUMP] '%s' (PID %d) Segfault at 0x%x. ERR: 0x%x. EIP: 0x%x", 
                current_task->name, current_task->id, cr2, regs->err_code, regs->eip);
        klog(dump);
        
        char dump_regs[256];
        sprintf(dump_regs, "[CRASH REGS] EAX:0x%x EBX:0x%x ECX:0x%x EDX:0x%x ESP:0x%x EBP:0x%x",
                regs->eax, regs->ebx, regs->ecx, regs->edx, regs->esp, regs->ebp);
        klog(dump_regs);
        
        klog_save();
        exit_process(); 
    } else {
        
        panic("Kernel Panic: Page Fault inside Kernel Ring 0!");
    }
}

void invalid_opcode_handler_c(struct registers_t *regs) {
    if (current_task && current_task->id > 1) {
        char dump[128];
        sprintf(dump, "[CRASH] Process '%s' (PID %d) Killed: Invalid Opcode!", current_task->name, current_task->id);
        klog(dump);
        klog_save();
        exit_process();
    } else {
        panic("Kernel Panic: Invalid Opcode inside Kernel!");
    }
}
