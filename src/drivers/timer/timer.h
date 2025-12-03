#ifndef TIMER_H
#define TIMER_H

#include "interrupt/idt/idt.h"

void timer_handler_c(void);
unsigned long get_ticks(void);

#endif