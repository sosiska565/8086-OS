#ifndef FONT_H
#define FONT_H

#include <stdint.h>

extern char font8x8_basic[1104][8];
extern uint8_t font_widths[1104];

void font_calc_widths(void);
int font_get_width(unsigned int c);
uint32_t blend_colors(uint32_t fg, uint32_t bg, float alpha);

#endif