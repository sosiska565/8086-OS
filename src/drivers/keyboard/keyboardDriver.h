#ifndef KEYBOARDDRIVER_H
#define KEYBOARDDRIVER_H

#include <stdint.h>
#include "interrupt/idt/idt.h"
#include "drivers/vga/vga.h"

void keyboard_handler_c(void);

#endif