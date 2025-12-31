#include "drivers/timer/timer.h"
#include "drivers/io/io.h"
#include "drivers/vga/vga.h"

volatile unsigned long ticks = 0;

void timer_handler_c(void){
    ticks++;
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