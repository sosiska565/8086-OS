#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <stdint.h>

void draw_rect(int x, int y, int w, int h, uint32_t color);
void draw_rect_filled(int x, int y, int w, int h, uint32_t color);
void draw_line(int x1, int y1, int x2, int y2, uint32_t color);
void draw_circle(int x0, int y0, int radius, uint32_t color);
void draw_circle_filled(int x0, int y0, int radius, uint32_t color);

#endif