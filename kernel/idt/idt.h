/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/kernel/idt/idt.h
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */

#ifndef IDT_H
#define IDT_H

#include<stdint.h>

void idt_set_gate(uint8_t num, uint32_t handler);
void idt_install(void);
void idt_init(void);
void pic_remap(void);

#endif
