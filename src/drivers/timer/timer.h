#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

void timer_handler_c(void);
unsigned long get_ticks(void);
void timer_install(void);
void sleep(unsigned long ms);
uint32_t get_cpu_usage(void);

#endif