#ifndef TIMER_H
#define TIMER_H

#include "drivers/io/io.h"

void timer_handler_c(void);
unsigned long get_ticks(void);
void timer_install(void);

#endif