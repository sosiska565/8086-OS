#ifndef TIMER_H
#define TIMER_H

void timer_handler_c(void);
unsigned long get_ticks(void);
void timer_install(void);
void sleep(unsigned long ms);

#endif