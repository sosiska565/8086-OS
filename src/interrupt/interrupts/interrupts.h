#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include "drivers/vga/vga.h"

extern struct interrupt_frame;

void system_division_handler_c(struct interrupt_frame *frame);

#endif