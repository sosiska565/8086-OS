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

void page_fault_handler_c(struct registers_t *regs) {
    uint32_t cr2;
    __asm__ volatile("mov %%cr2, %0" : "=r"(cr2)); 

    if (current_task && current_task->id > 1) {
        set_text_color(4); 
        printf("\n========================================\n");
        printf("  [CRASH DUMP] PROCESS SEGFAULT (PID %d)\n", current_task->id);
        printf("========================================\n");
        set_text_color(15); 
        
        printf(" Process Name : %s\n", current_task->name);
        printf(" Fault Address: 0x%x (CR2)\n", cr2);
        printf(" Error Code   : 0x%x\n\n", regs->err_code);
        
        printf(" --- CPU REGISTERS ---\n");
        printf(" EIP: 0x%x | ESP: 0x%x | EBP: 0x%x\n", regs->eip, regs->esp, regs->ebp);
        printf(" EAX: 0x%x | EBX: 0x%x | ECX: 0x%x\n", regs->eax, regs->ebx, regs->ecx);
        printf(" EDX: 0x%x | ESI: 0x%x | EDI: 0x%x\n", regs->edx, regs->esi, regs->edi);

        
        printf("\n --- KERNEL HALTED PROCESS SAFELY ---\n");

        
        char crash_msg[128] = "";
        char buff[50] = "";
        strcat(crash_msg, "[CRASH] Process ");
        strcat(crash_msg, current_task->name);
        strcat(crash_msg, "(PID ");
        itoa(current_task->id, buff,10);
        strcat(crash_msg, buff);
        strcat(crash_msg, ") Segfault at 0x");
        itoa(cr2, buff, 16);
        strcat(crash_msg, buff);
        
        klog(crash_msg);
        klog_save();

        set_text_color(7); 
        exit_process(); 
    } else {
        panic("Kernel Panic: Page Fault inside Kernel Ring 0!");
    }
}

void invalid_opcode_handler_c(struct registers_t *regs) {
    if (current_task && current_task->id > 1) {
        printf("\n%C[CRASH] Process PID %d Killed: Invalid Opcode (Unsupported CPU instruction)!%C\n", VGA32_COLOR_RED, current_task->id, VGA32_COLOR_WHITE);
        exit_process();
    } else {
        panic("Kernel Panic: Invalid Opcode inside Kernel!");
    }
}