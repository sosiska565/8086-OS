#ifndef KEYBOARDDRIVER_H
#define KEYBOARDDRIVER_H

#include <stdint.h>

typedef struct {
    uint8_t scancode;
    char lower;
    char upper;
    char shift_alt;
    char caps;
    char *desc;
} Scancode_entity;

void keyboard_handler_c(void);
char getch(void);
void gets(char* buffer, int max_len);
uint8_t wait_scancode(void);
void keyboard_flush(void);
char scancode_to_char(uint8_t scancode);

#endif