#ifndef MOUSE_H
#define MOUSE_H

#include <stdint.h>

void mouse_init();
void mouse_handler_c();

extern int mouse_x;
extern int mouse_y;
extern uint8_t mouse_left_pressed;
extern uint8_t mouse_right_pressed;

#endif