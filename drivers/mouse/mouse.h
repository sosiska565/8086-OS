#ifndef MOUSE_H
#define MOUSE_H

#include <stdint.h>

typedef struct {
    int x;
    int y;
    uint8_t left_button;
    uint8_t right_button;
    uint8_t middle_button;
} MouseState;

void mouse_init(void);
void mouse_handler_c(void);

extern MouseState mouse;

#endif