#include"drivers/keyboard/keyboardDriver.h"

void keyboard_handler_c(void){
    outb(0x20, 0x20);
}