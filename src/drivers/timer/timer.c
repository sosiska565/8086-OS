#include "drivers/timer/timer.h"
#include "drivers/io/io.h"
#include "drivers/vga/vga.h"
#include "multitask/task.h"
#include "drivers/video/vesa.h"

volatile unsigned long ticks = 0;
static int debug_ticks = 0;

void timer_handler_c(void){
    ticks++;

    if(ticks % 5 == 0){
        check_kill_flag();
        task_scheduler();
    }
    

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

void sleep(unsigned long ms) {
    unsigned long end_ticks = get_ticks() + ms;
    while (get_ticks() < end_ticks) {
        printf("%d", get_ticks());
    }
}