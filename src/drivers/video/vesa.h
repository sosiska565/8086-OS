#ifndef VESA_H
#define VESA_H

#include <stdint.h>

void init_vesa(void);
void put_pixel(int x, int y, uint32_t color);
void clear_screen_vesa(uint32_t color);
void vesa_draw_char(int x, int y, char c, uint32_t color, uint32_t bgcolor);
int get_screen_width(void);
int get_screen_height(void);

#endif