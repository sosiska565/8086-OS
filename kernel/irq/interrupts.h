/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/kernel/irq/interrupts.h
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */

#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <stdint.h>

typedef struct registers_t {
    uint32_t ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, useresp, ss;
} registers_t;

extern struct interrupt_frame;

void system_division_handler_c(struct interrupt_frame *frame);

#endif
