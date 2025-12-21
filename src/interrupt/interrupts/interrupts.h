#ifndef INTERRUPTS_H
#define INTERRUPTS_H

extern struct interrupt_frame;

void system_division_handler_c(struct interrupt_frame *frame);

#endif