/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/kernel/syscalls/syscalls.h
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */

#ifndef SYSCALLS_H
#define SYSCALLS_H

#include <stdint.h>

struct syscall_registers {
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
};

void syscall_handler_c(struct syscall_registers *regs);

#endif
