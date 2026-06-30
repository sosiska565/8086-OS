/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/drivers/keyboard/keyboardDriver.h
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */

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

extern uint8_t current_layout;

void keyboard_handler_c(void);
unsigned int getch(void);
void gets(char* buffer, int max_len);
uint8_t wait_scancode(void);
void keyboard_flush(void);
unsigned int scancode_to_char(uint8_t scancode);
unsigned int scancode_to_char_layout(uint8_t scancode);
void keyboard_init(void);
unsigned int poll_buffer(void);
uint8_t get_keyboard_modifiers(void);

#endif
