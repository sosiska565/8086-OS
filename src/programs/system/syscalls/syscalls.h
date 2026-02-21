#ifndef SYSCALLS_H
#define SYSCALLS_H

#include <stdint.h>

struct syscall_registers {
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
};

void syscall_handler_c(struct syscall_registers *regs);

#endif