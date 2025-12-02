#ifndef KEYBOARDDRIVER_H
#define KEYBOARDDRIVER_H

#include <stdint.h>
#include <stddef.h>
#include "interrupt/idt/idt.h"
#include "drivers/vga/vga.h"

typedef struct scancode_entity {
    unsigned int scancode;
    char normal;
    char shift;
    char altgr;
    char caps;
    const char* name;
} Scancode_entity;

void keyboard_handler_c(void);
int get_scancode(void);
Scancode_entity* get_key(int scancode);
char getch(void);
void gets(char* buffer, int max_len);

#endif