#ifndef KEYBOARDDRIVER_H
#define KEYBOARDDRIVER_H

#include <stdint.h>

typedef struct {
    uint8_t scancode;
    unsigned int lower;
    unsigned int upper;
    unsigned int shift_alt;
    unsigned int caps;
    char *desc;
} Scancode_entity;

void keyboard_handler_c(void);
unsigned int getch(void);
void gets(char* buffer, int max_len);
uint8_t wait_scancode(void);
void keyboard_flush(void);
unsigned int scancode_to_char(uint8_t scancode);
unsigned int scancode_to_char_layout(uint8_t scancode);

#endif