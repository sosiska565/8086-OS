#include "drivers/speaker/speaker.h"

void beep(unsigned int frequency) {
    if (frequency == 0) frequency = 1000;
    
    unsigned int divisor = 1193180 / frequency;
    
    outb(0x43, 0xB6);
    
    outb(0x42, divisor & 0xFF);
    outb(0x42, (divisor >> 8) & 0xFF);
    
    unsigned char tmp = inb(0x61);
    outb(0x61, tmp | 0x03);
}

void stop_beep(void) {
    unsigned char tmp = inb(0x61);
    outb(0x61, tmp & 0xFC);
}

void beep_timed(unsigned int frequency, unsigned int duration_ms) {
    beep(frequency);
    
    for (volatile unsigned long i = 0; i < (duration_ms * 1000); i++);
    
    stop_beep();
}