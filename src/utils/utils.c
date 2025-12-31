#include "utils/utils.h"
#include "drivers/vga/vga.h"

static unsigned long next = 1;

void srand(unsigned long seed){
    next = seed;
}

unsigned long random(void){
    unsigned long m = 1UL << 31; 
    next = (1664525 * next + 1013904223) % m;
    return next;
}