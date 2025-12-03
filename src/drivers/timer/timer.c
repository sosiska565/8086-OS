#include "drivers/timer/timer.h"

unsigned long ticks = 0;

void timer_handler_c(void){
    ticks++;
    outb(0x20, 0x20);
}

unsigned long get_ticks(void){
    return ticks;
}