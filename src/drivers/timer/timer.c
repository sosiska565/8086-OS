#include "drivers/timer/timer.h"
#include "drivers/io/io.h"

unsigned long ticks = 0;

void timer_handler_c(void){
    ticks++;
    outb(0x20, 0x20);
}

unsigned long get_ticks(void){
    return ticks;
}

void timer_install(void) {
    unsigned int divisor = 1193180 / 1000;
    
    outb(0x43, 0x36);
    
    outb(0x40, divisor & 0xFF);       
    outb(0x40, (divisor >> 8) & 0xFF);
}