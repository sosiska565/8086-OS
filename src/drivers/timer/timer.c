#include "drivers/timer/timer.h"

void timer_handler_c(void){
    //print("timer_handler_c!!!!!");
    outb(0x20, 0x20);
}